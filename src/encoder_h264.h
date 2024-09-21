#pragma once

#include "encoder.h"
#include "bitstream_h264.h"

#include <va/va.h>
#include <va/va_enc_h264.h>

class encoder_h264 : public enc_encoder
{
public:
   encoder_h264();

   bool create(const struct enc_encoder_params *params) override;
   struct enc_task *encode_frame(const struct enc_frame_params *params) override;

private:
   bitstream_h264::sps sps = {};
   bitstream_h264::pps pps = {};
   bitstream_h264::slice slice = {};

   struct dpb_entry {
      bool valid = false;
      uint64_t frame_id = 0;
      uint32_t pic_order_cnt = 0;
   };
   std::vector<dpb_entry> dpb;

   uint64_t frame_id = 0;
   uint32_t gop_size = 0;
   uint64_t gop_count = 0;
   uint32_t pic_order_cnt = 0;
   uint16_t idr_pic_id = 0;
};
