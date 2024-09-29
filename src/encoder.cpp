#include "encoder.h"
#include "surface.h"

#include <assert.h>
#include <string.h>
#include <iostream>
#include <algorithm>

enc_encoder::enc_encoder()
{
}

enc_encoder::~enc_encoder()
{
   for (auto &b : buffer_pool)
      vaDestroyBuffer(dpy, b.second);
   for (auto &d : dpb)
      vaDestroySurfaces(dpy, &d.surface, 1);
   if (context_id != VA_INVALID_ID)
      vaDestroyContext(dpy, context_id);
   if (config_id != VA_INVALID_ID)
      vaDestroyConfig(dpy, config_id);
}

bool enc_encoder::create_context(const struct enc_encoder_params *params, std::vector<VAConfigAttrib> &attribs)
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

   initial_bit_rate = params->rc_params[0].bit_rate;

   int32_t num_entrypoints = 0;
   std::vector<VAEntrypoint> entrypoints(vaMaxNumEntrypoints(dpy));
   vaQueryConfigEntrypoints(dpy, profile, entrypoints.data(), &num_entrypoints);
   for (int32_t i = 0; i < num_entrypoints; i++) {
      if (entrypoints[i] == VAEntrypointEncSlice || entrypoints[i] == VAEntrypointEncSliceLP) {
         entrypoint = entrypoints[i];
         break;
      }
   }
   if (!entrypoint) {
      std::cerr << "No encode entrypoint available for profile " << profile << std::endl;
      return false;
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

   attrib.type = VAConfigAttribEncPackedHeaders;
   attrib.value = get_config_attrib(VAConfigAttribEncPackedHeaders);
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

   if (params->low_latency) {
      if (params->dev->vendor == enc_dev::VENDOR_AMD)
         setenv("AMD_DEBUG", "lowlatencyenc", 1);
      /*
      attrib.type = VAConfigAttribLowLatency;
      attrib.value = 1;
      attribs.push_back(attrib);
      */
   }

   VAStatus status = vaCreateConfig(dpy, profile, entrypoint, attribs.data(), attribs.size(), &config_id);
   if (!va_check(status, "vaCreateConfig"))
      return false;

   status = vaCreateContext(dpy, config_id, aligned_width, aligned_height, VA_PROGRESSIVE, nullptr, 0, &context_id);
   if (!va_check(status, "vaCreateContext"))
      return false;

   num_refs = params->num_refs;
   dpb.resize(num_refs + 1);
   for (auto &d : dpb) {
      status = vaCreateSurfaces(dpy, rt_format, aligned_width, aligned_height, &d.surface, 1, nullptr, 0);
      if (!va_check(status, "vaCreateSurfaces"))
         return false;
   }

   codedbuf_size = aligned_width * aligned_height * 3 + (1 << 16);

   num_layers = params->num_rc_layers ? params->num_rc_layers : 1;
   update_rate_control(params->rc_params);

   return true;
}

std::unique_ptr<enc_task> enc_encoder::begin_encode(const struct enc_frame_params *params)
{
   // Pick frame type
   enc_params.frame_type = params->frame_type;
   if (enc_params.frame_type == ENC_FRAME_TYPE_UNKNOWN) {
      if (enc_params.frame_id == 0 || (!intra_refresh && gop_size != 0 && enc_params.gop_count == gop_size))
         enc_params.frame_type = ENC_FRAME_TYPE_IDR;
      else
         enc_params.frame_type = ENC_FRAME_TYPE_P;
   }

   if (enc_params.frame_type == ENC_FRAME_TYPE_IDR) {
      enc_params.frame_id = 0;
      enc_params.gop_count = 0;
      for (auto &d : dpb)
         d.reset();
   }

   // Invalidate requested refs
   for (uint32_t i = 0; i < params->num_invalidate_refs; i++) {
      uint64_t invalidate_id = params->invalidate_refs[i];
      for (auto &d : dpb) {
         if (d.ok() && (d.frame_id == invalidate_id || std::find(d.refs.begin(), d.refs.end(), invalidate_id) != d.refs.end())) {
            d.available = false;
            break;
         }
      }
   }

   // Find ref_l0
   enc_params.ref_l0_slot = 0xff;
   if (enc_params.frame_type == ENC_FRAME_TYPE_P) {
      // Use requested refs
      if (params->num_ref_list0) {
         uint64_t frame_id = params->ref_list0[0];
         for (uint32_t i = 0; i < dpb.size(); i++) {
            if (dpb[i].ok() && dpb[i].frame_id == frame_id) {
               enc_params.ref_l0_slot = i;
               break;
            }
         }
      }

      if (enc_params.ref_l0_slot == 0xff && num_refs > 0) {
         uint64_t highest_id = 0;
         // Prefer short term reference
         for (uint32_t i = 0; i < dpb.size(); i++) {
            if (dpb[i].ok() && !dpb[i].long_term && dpb[i].frame_id >= highest_id) {
               highest_id = dpb[i].frame_id;
               enc_params.ref_l0_slot = i;
            }
         }
         // Use long term references
         if (enc_params.ref_l0_slot == 0xff) {
            for (uint32_t i = 0; i < dpb.size(); i++) {
               if (dpb[i].ok() && dpb[i].frame_id >= highest_id) {
                  highest_id = dpb[i].frame_id;
                  enc_params.ref_l0_slot = i;
               }
            }
         }
      }

      // No ref found, use I frame
      if (enc_params.ref_l0_slot == 0xff)
         enc_params.frame_type = ENC_FRAME_TYPE_I;
   }

   // Find available slot
   enc_params.recon_slot = 0xff;
   for (uint32_t i = 0; i < dpb.size(); i++) {
      if (!dpb[i].valid) {
         enc_params.recon_slot = i;
         break;
      }
   }

   enc_params.referenced = num_refs > 0 && (enc_params.frame_type != ENC_FRAME_TYPE_P || !params->not_referenced);
   enc_params.long_term = enc_params.referenced && params->long_term;
   enc_params.need_sequence_headers = enc_params.frame_type == ENC_FRAME_TYPE_IDR || (intra_refresh && enc_params.gop_count % gop_size == 0);
   enc_params.is_recovery_point = intra_refresh && enc_params.need_sequence_headers && enc_params.frame_id > 0;

   if (enc_params.long_term) {
      uint8_t long_term = 0;
      for (auto &d : dpb) {
         if (d.valid && d.long_term)
            long_term++;
      }
      if (long_term == num_refs - 1)
         enc_params.long_term = false;
   }

   assert(enc_params.recon_slot != 0xff);
   dpb[enc_params.recon_slot].valid = enc_params.referenced;
   dpb[enc_params.recon_slot].available = enc_params.referenced;
   dpb[enc_params.recon_slot].frame_id = enc_params.frame_id;
   dpb[enc_params.recon_slot].long_term = enc_params.long_term;
   if (enc_params.ref_l0_slot != 0xff && enc_params.referenced) {
      auto &refs = dpb[enc_params.ref_l0_slot].refs;
      auto &current = dpb[enc_params.recon_slot].refs;
      current.insert(current.end(), refs.size() < 16 ? refs.begin() : refs.begin() + 1, refs.end());
      current.push_back(dpb[enc_params.ref_l0_slot].frame_id);
   }

   VAStatus status = vaBeginPicture(dpy, context_id, params->surface->surface_id);
   if (!va_check(status, "vaBeginPicture"))
      return {};

   return std::make_unique<enc_task>(this);
}

bool enc_encoder::end_encode(const struct enc_frame_params *params)
{
   if (params->rc_params)
      update_rate_control(params->rc_params);

   if (intra_refresh)
      update_intra_refresh();

   VAStatus status = vaRenderPicture(dpy, context_id, pic_buffers.data(), pic_buffers.size());

   for (VABufferID buf : pic_buffers)
      vaDestroyBuffer(dpy, buf);
   pic_buffers.clear();

   if (!va_check(status, "vaRenderPicture"))
      return false;

   status = vaEndPicture(dpy, context_id);
   if (!va_check(status, "vaEndPicture"))
      return false;

   // Invalidate oldest reference
   if (num_refs > 0) {
      uint8_t used_refs = 0;
      for (auto &d : dpb) {
         if (d.valid)
            used_refs++;
      }

      if (used_refs > num_refs) {
         uint8_t slot = 0xff;
         uint64_t lowest_id = UINT64_MAX;
         for (uint32_t i = 0; i < dpb.size(); i++) {
            if (i != enc_params.recon_slot && dpb[i].valid && !dpb[i].long_term && dpb[i].frame_id < lowest_id) {
               lowest_id = dpb[i].frame_id;
               slot = i;
            }
         }
         assert(slot != 0xff);
         dpb[slot].reset();
      }
   }

   if (params->feedback) {
      memset(params->feedback, 0, sizeof(*params->feedback));
      params->feedback->frame_type = enc_params.frame_type;
      params->feedback->frame_id = enc_params.frame_id;
      params->feedback->referenced = enc_params.referenced;
      params->feedback->long_term = enc_params.long_term;
      if (enc_params.ref_l0_slot != 0xff) {
         params->feedback->num_ref_list0 = 1;
         params->feedback->ref_list0[0] = dpb[enc_params.ref_l0_slot].frame_id;
      }
      for (auto &d : dpb) {
         if (d.ok())
            params->feedback->ref[params->feedback->num_refs++] = d.frame_id;
      }
   }

   if (enc_params.referenced)
      enc_params.frame_id++;

   enc_params.gop_count++;

   return true;
}

uint32_t enc_encoder::get_config_attrib(VAConfigAttribType type)
{
   VAConfigAttrib attrib;
   attrib.type = type;
   VAStatus status = vaGetConfigAttributes(dpy, profile, entrypoint, &attrib, 1);
   if (!va_check(status, "vaGetConfigAttributes"))
      return 0;
   return attrib.value;
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
      auto framerate = get_framerate(params[i].frame_rate);
      VAEncMiscParameterFrameRate fr = {};
      fr.framerate = framerate.first | (framerate.second << 16);
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
   rir.intra_insertion_location = (enc_params.gop_count % gop_size) * rir.intra_insert_size;
   add_misc_buffer(VAEncMiscParameterTypeRIR, sizeof(rir), &rir);
}
