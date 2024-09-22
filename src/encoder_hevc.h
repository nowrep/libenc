#pragma once

#include "encoder.h"
#include "bitstream_hevc.h"

#include <va/va.h>
#include <va/va_enc_hevc.h>

class encoder_hevc : public enc_encoder
{
public:
   encoder_hevc();

   bool create(const struct enc_encoder_params *params) override;
   struct enc_task *encode_frame(const struct enc_frame_params *params) override;

private:
   bitstream_hevc::vps vps = {};
   bitstream_hevc::sps sps = {};
   bitstream_hevc::pps pps = {};
   bitstream_hevc::slice slice = {};

   uint16_t idr_pic_id = 0;
   uint64_t pic_order_cnt = 0;
   std::vector<uint64_t> dpb_poc;
};
