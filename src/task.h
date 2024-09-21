#pragma once

#include "encoder.h"

struct enc_task
{
   enc_task(enc_encoder *encoder);
   virtual ~enc_task();

   bool wait(uint64_t timeout_ns);
   bool get_bitstream(uint32_t *size, uint8_t **data);

   enc_encoder *encoder;
   VABufferID buffer_id = VA_INVALID_ID;
   VACodedBufferSegment *coded_buf = nullptr;
};
