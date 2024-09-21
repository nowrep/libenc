#include "encoder.h"
#include "surface.h"

#include <string.h>
#include <iostream>

enc_encoder::enc_encoder()
{
}

enc_encoder::~enc_encoder()
{
   if (!dpb_surfaces.empty())
      vaDestroySurfaces(dpy, dpb_surfaces.data(), dpb_surfaces.size());
   if (context_id != VA_INVALID_ID)
      vaDestroyContext(dpy, context_id);
   if (config_id != VA_INVALID_ID)
      vaDestroyConfig(dpy, config_id);
}

bool enc_encoder::create_context(const struct enc_encoder_params *params, VAProfile profile, std::vector<VAConfigAttrib> &attribs)
{
   if (!aligned_width || !aligned_height)
      return false;

   dpy = params->dev->dpy;

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

   update_frame_rate(params->frame_rate.num, params->frame_rate.den);
   if (params->rc_params)
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
   return va_check(status, "vaEndPicture");
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
   size += sizeof(VAEncMiscParameterBuffer);
   VAStatus status = vaCreateBuffer(dpy, context_id, VAEncMiscParameterBufferType, size, 1, nullptr, &buf_id);
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

void enc_encoder::update_frame_rate(uint32_t num, uint32_t den)
{
   VAEncMiscParameterFrameRate fr = {};
   fr.framerate = num | (den << 16);
   add_misc_buffer(VAEncMiscParameterTypeFrameRate, sizeof(fr), &fr);
}

void enc_encoder::update_rate_control(const struct enc_rate_control_params *params)
{
   VAEncMiscParameterRateControl rc = {};
   rc.bits_per_second = std::max(params->bit_rate, params->peak_bit_rate);
   rc.target_percentage = (rc.bits_per_second * 100) / params->bit_rate;
   rc.min_qp = params->min_qp;
   rc.max_qp = params->max_qp;
   rc.quality_factor = params->qvbr_quality;
   rc.rc_flags.bits.disable_bit_stuffing = params->disable_filler_data;
   add_misc_buffer(VAEncMiscParameterTypeRateControl, sizeof(rc), &rc);

   VAEncMiscParameterHRD hrd = {};
   hrd.buffer_size = params->vbv_buffer_size;
   hrd.initial_buffer_fullness = params->vbv_initial_fullness;
   add_misc_buffer(VAEncMiscParameterTypeHRD, sizeof(hrd), &hrd);

   if (params->max_frame_size) {
      VAEncMiscParameterBufferMaxFrameSize size = {};
      size.max_frame_size = params->max_frame_size;
      add_misc_buffer(VAEncMiscParameterTypeMaxFrameSize, sizeof(size), &size);
   }
}
