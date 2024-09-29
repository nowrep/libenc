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

   bool create_context(const struct enc_encoder_params *params, std::vector<VAConfigAttrib> &attribs);
   std::unique_ptr<enc_task> begin_encode(const struct enc_frame_params *params);
   bool end_encode(const struct enc_frame_params *params);

   uint32_t get_config_attrib(VAConfigAttribType type);

   void add_buffer(VABufferType type, uint32_t size, const void *data);
   void add_misc_buffer(VAEncMiscParameterType type, uint32_t size, const void *data);
   void add_packed_header(uint32_t type, const bitstream &bs);

   VABufferID acquire_buffer();
   void release_buffer(VABufferID buffer);

   void update_rate_control(const struct enc_rate_control_params *params);
   void update_intra_refresh();

   VADisplay dpy;
   VAConfigID config_id = VA_INVALID_ID;
   VAContextID context_id = VA_INVALID_ID;
   VAProfile profile = {};
   VAEntrypoint entrypoint = {};
   uint32_t rt_format = 0;
   uint32_t unit_width = 0;
   uint32_t unit_height = 0;
   uint32_t aligned_width = 0;
   uint32_t aligned_height = 0;
   uint32_t codedbuf_size = 0;
   std::vector<VABufferID> pic_buffers;
   std::vector<std::pair<bool, VABufferID>> buffer_pool;

   uint32_t num_refs = 0;
   uint32_t num_layers = 0;
   uint32_t gop_size = 0;
   bool intra_refresh = false;
   uint32_t initial_bit_rate = 0;

   struct {
      enum enc_frame_type frame_type = ENC_FRAME_TYPE_UNKNOWN;
      uint64_t frame_id = 0;
      uint32_t recon_slot = 0;
      uint32_t ref_l0_slot = 0;
      uint64_t gop_count = 0;
      bool referenced = false;
      bool long_term = false;
      bool need_sequence_headers = false;
      bool is_recovery_point = false;
   } enc_params;

   struct dpb_entry {
      bool valid = false;
      bool available = false;
      VASurfaceID surface = VA_INVALID_SURFACE;
      uint64_t frame_id = 0;
      bool long_term = false;
      std::vector<uint64_t> refs;

      bool ok() const {
         return valid && available;
      }

      void reset() {
         valid = false;
         available = false;
         frame_id = 0;
         long_term = false;
         refs.clear();
      }
   };
   std::vector<dpb_entry> dpb;
};
