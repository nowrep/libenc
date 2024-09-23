#pragma once

#include "encoder.h"
#include "bitstream_av1.h"

#include <va/va.h>
#include <va/va_enc_av1.h>

class encoder_av1 : public enc_encoder
{
public:
   encoder_av1();

   bool create(const struct enc_encoder_params *params) override;
   struct enc_task *encode_frame(const struct enc_frame_params *params) override;

private:
   bitstream_av1::seq seq = {};
   bitstream_av1::frame frame = {};

   uint32_t ref_idx_slot = 0;
   std::vector<uint32_t> dpb_ref_idx;
};