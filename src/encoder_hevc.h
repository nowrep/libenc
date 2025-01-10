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

   uint32_t write_sps(uint8_t *buf, uint32_t buf_size) override;
   uint32_t write_pps(uint8_t *buf, uint32_t buf_size) override;
   uint32_t write_vps(uint8_t *buf, uint32_t buf_size) override;

private:
   bitstream_hevc::vps vps = {};
   bitstream_hevc::sps sps = {};
   bitstream_hevc::pps pps = {};
   bitstream_hevc::slice slice = {};

   VAConfigAttribValEncHEVCFeatures features = {};
   VAConfigAttribValEncHEVCBlockSizes block_sizes = {};

   uint16_t idr_pic_id = 0;
   uint64_t pic_order_cnt = 0;
};
