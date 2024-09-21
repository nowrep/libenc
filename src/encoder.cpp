#include "encoder.h"
#include "surface.h"

#include <string.h>
#include <iostream>

enc_encoder::enc_encoder()
{
}

enc_encoder::~enc_encoder()
{
   for (auto &b : buffer_pool)
      vaDestroyBuffer(dpy, b.second);
   if (!dpb_surfaces.empty())
      vaDestroySurfaces(dpy, dpb_surfaces.data(), dpb_surfaces.size());
   if (context_id != VA_INVALID_ID)
      vaDestroyContext(dpy, context_id);
   if (config_id != VA_INVALID_ID)
      vaDestroyConfig(dpy, config_id);
}

bool enc_encoder::create_context(const struct enc_encoder_params *params, VAProfile profile, std::vector<VAConfigAttrib> &attribs)
{
   dpy = params->dev->dpy;

   aligned_width = align(params->width, unit_width);
   aligned_height = align(params->height, unit_height);

   if (params->intra_refresh) {
      intra_refresh = true;
      gop_size = std::min(params->gop_size, aligned_width / unit_width);
   } else {
      gop_size = params->gop_size;
   }

   VAConfigAttrib attrib;
   attrib.type = VAConfigAttribRTFormat;
   switch (params->bit_depth) {
   case 8:
      rt_format = VA_RT_FORMAT_YUV420;
      break;
   case 10:
      rt_format = VA_RT_FORMAT_YUV420_10;
      break;
   default:
      std::cerr << "Invalid bit depth value: " << params->bit_depth << std::endl;
      return false;
   }
   attrib.value = rt_format;
   attribs.push_back(attrib);

   attrib.type = VAConfigAttribRateControl;
   switch (params->rc_mode) {
   case ENC_RATE_CONTROL_MODE_CQP:
      attrib.value = VA_RC_CQP;
      break;
   case ENC_RATE_CONTROL_MODE_CBR:
      attrib.value = VA_RC_CBR;
      break;
   case ENC_RATE_CONTROL_MODE_VBR:
      attrib.value = VA_RC_VBR;
      break;
   case ENC_RATE_CONTROL_MODE_QVBR:
      attrib.value = VA_RC_QVBR;
      break;
   default:
      std::cerr << "Invalid rc_mode value: " << params->rc_mode << std::endl;
      return false;
   }
   attribs.push_back(attrib);

   num_refs = params->num_refs;
   dpb_surfaces.resize(num_refs + 1);
   VAStatus status = vaCreateSurfaces(dpy, rt_format, aligned_width, aligned_height, dpb_surfaces.data(), dpb_surfaces.size(), nullptr, 0);
   if (!va_check(status, "vaCreateSurfaces"))
      return false;

   status = vaCreateConfig(dpy, profile, VAEntrypointEncSlice, attribs.data(), attribs.size(), &config_id);
   if (!va_check(status, "vaCreateConfig"))
      return false;

   status = vaCreateContext(dpy, config_id, aligned_width, aligned_height, VA_PROGRESSIVE, nullptr, 0, &context_id);
   if (!va_check(status, "vaCreateContext"))
      return false;

   codedbuf_size = aligned_width * aligned_height * 3 + (1 << 16);

   num_layers = params->num_rc_layers ? params->num_rc_layers : 1;
   update_rate_control(params->rc_params);

   return true;
}

std::unique_ptr<enc_task> enc_encoder::begin_encode(const struct enc_frame_params *params)
{
   VAStatus status = vaBeginPicture(dpy, context_id, params->surface->surface_id);
   if (!va_check(status, "vaBeginPicture"))
      return {};

   if (params->rc_params)
      update_rate_control(params->rc_params);

   if (intra_refresh)
      update_intra_refresh();

   return std::make_unique<enc_task>(this);
}

bool enc_encoder::end_encode()
{
   VAStatus status = vaRenderPicture(dpy, context_id, pic_buffers.data(), pic_buffers.size());

   for (VABufferID buf : pic_buffers)
      vaDestroyBuffer(dpy, buf);
   pic_buffers.clear();

   if (!va_check(status, "vaRenderPicture"))
      return false;

   status = vaEndPicture(dpy, context_id);
   if (!va_check(status, "vaEndPicture"))
      return false;

   gop_count = (gop_count + 1) % gop_size;

   return true;
}

void enc_encoder::add_buffer(VABufferType type, uint32_t size, const void *data)
{
   VABufferID buf_id;
   VAStatus status = vaCreateBuffer(dpy, context_id, type, size, 1, const_cast<void *>(data), &buf_id);
   if (!va_check(status, "vaCreateBuffer"))
      return;
   pic_buffers.push_back(buf_id);
}

void enc_encoder::add_misc_buffer(VAEncMiscParameterType type, uint32_t size, const void *data)
{
   VABufferID buf_id;
   VAStatus status = vaCreateBuffer(dpy, context_id, VAEncMiscParameterBufferType, size + sizeof(VAEncMiscParameterBuffer), 1, nullptr, &buf_id);
   if (!va_check(status, "vaCreateBuffer"))
      return;

   VAEncMiscParameterBuffer *param;
   status = vaMapBuffer2(dpy, buf_id, reinterpret_cast<void**>(&param), VA_MAPBUFFER_FLAG_WRITE);
   if (!va_check(status, "vaMapBuffer2"))
      return;

   param->type = type;
   memcpy(param->data, data, size);
   vaUnmapBuffer(dpy, buf_id);

   pic_buffers.push_back(buf_id);
}

void enc_encoder::add_packed_header(uint32_t type, const bitstream &bs)
{
   VAEncPackedHeaderParameterBuffer param = {};
   param.type = type;
   param.bit_length = bs.size() * 8;
   add_buffer(VAEncPackedHeaderParameterBufferType, sizeof(param), &param);
   add_buffer(VAEncPackedHeaderDataBufferType, bs.size(), bs.data());
}

VABufferID enc_encoder::acquire_buffer()
{
   for (auto &b : buffer_pool) {
      if (b.first) {
         b.first = false;
         return b.second;
      }
   }

   VABufferID buf_id;
   VAStatus status = vaCreateBuffer(dpy, context_id, VAEncCodedBufferType, codedbuf_size, 1, nullptr, &buf_id);
   if (!va_check(status, "vaCreateBuffer"))
      return VA_INVALID_ID;
   buffer_pool.push_back({false, buf_id});
   return buf_id;
}

void enc_encoder::release_buffer(VABufferID buffer)
{
   for (auto &b : buffer_pool) {
      if (b.second == buffer) {
         b.first = true;
         break;
      }
   }
}

void enc_encoder::update_rate_control(const struct enc_rate_control_params *params)
{
   if (num_layers > 1) {
      VAEncMiscParameterTemporalLayerStructure layer = {};
      layer.number_of_layers = num_layers;
      add_misc_buffer(VAEncMiscParameterTypeTemporalLayerStructure, sizeof(layer), &layer);
   }

   for (uint32_t i = 0; i < num_layers; i++) {
      uint32_t num = params[i].frame_rate;
      uint32_t den = 1;
      if (params[i].frame_rate - static_cast<uint32_t>(params[i].frame_rate) > 0.0001) {
         num = params[i].frame_rate * 1000;
         den = 1000;
      }
      VAEncMiscParameterFrameRate fr = {};
      fr.framerate = num | (den << 16);
      fr.framerate_flags.bits.temporal_id = i;
      add_misc_buffer(VAEncMiscParameterTypeFrameRate, sizeof(fr), &fr);

      VAEncMiscParameterRateControl rc = {};
      rc.bits_per_second = std::max(params[i].bit_rate, params[i].peak_bit_rate);
      rc.target_percentage = (rc.bits_per_second * 100) / params[i].bit_rate;
      rc.min_qp = params[i].min_qp;
      rc.max_qp = params[i].max_qp;
      rc.quality_factor = params[i].qvbr_quality;
      rc.rc_flags.bits.temporal_id = i;
      rc.rc_flags.bits.disable_bit_stuffing = params[i].disable_filler_data;
      add_misc_buffer(VAEncMiscParameterTypeRateControl, sizeof(rc), &rc);
   }

   VAEncMiscParameterHRD hrd = {};
   hrd.buffer_size = params[0].vbv_buffer_size;
   hrd.initial_buffer_fullness = params[0].vbv_initial_fullness;
   add_misc_buffer(VAEncMiscParameterTypeHRD, sizeof(hrd), &hrd);

   if (params[0].max_frame_size) {
      VAEncMiscParameterBufferMaxFrameSize size = {};
      size.max_frame_size = params[0].max_frame_size;
      add_misc_buffer(VAEncMiscParameterTypeMaxFrameSize, sizeof(size), &size);
   }
}

void enc_encoder::update_intra_refresh()
{
   VAEncMiscParameterRIR rir = {};
   rir.rir_flags.bits.enable_rir_column = 1;
   rir.intra_insert_size = aligned_width / unit_width / gop_size;
   rir.intra_insertion_location = gop_count * rir.intra_insert_size;
   add_misc_buffer(VAEncMiscParameterTypeRIR, sizeof(rir), &rir);
}
