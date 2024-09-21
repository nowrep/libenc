#pragma once

#include "dev.h"
#include "task.h"
#include "util.h"
#include "bitstream.h"
#include <vector>
#include <memory>

struct enc_encoder
{
   enc_encoder();
   virtual ~enc_encoder();

   virtual bool create(const enc_encoder_params *params) = 0;
   virtual struct enc_task *encode_frame(const struct enc_frame_params *params) = 0;

   bool create_context(const struct enc_encoder_params *params, VAProfile profile, std::vector<VAConfigAttrib> &attribs);
   std::unique_ptr<enc_task> begin_encode(const struct enc_frame_params *params);
   bool end_encode();

   void add_buffer(VABufferType type, uint32_t size, const void *data);
   void add_misc_buffer(VAEncMiscParameterType type, uint32_t size, const void *data);
   void add_packed_header(uint32_t type, const bitstream &bs);

   VABufferID acquire_buffer();
   void release_buffer(VABufferID buffer);

   void update_frame_rate(uint32_t num, uint32_t den);
   void update_rate_control(const struct enc_rate_control_params *params);

   VADisplay dpy;
   VAConfigID config_id = VA_INVALID_ID;
   VAContextID context_id = VA_INVALID_ID;
   uint32_t rt_format = 0;
   uint32_t aligned_width = 0;
   uint32_t aligned_height = 0;
   uint32_t codedbuf_size = 0;
   std::vector<VASurfaceID> dpb_surfaces;
   std::vector<VABufferID> pic_buffers;
   std::vector<std::pair<bool, VABufferID>> buffer_pool;

   uint32_t num_refs = 0;
   uint64_t frame_id = 0;
};
