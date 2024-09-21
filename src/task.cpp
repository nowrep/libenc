#include "task.h"

enc_task::enc_task(enc_encoder *encoder)
   : encoder(encoder)
{
   buffer_id = encoder->acquire_buffer();
}

enc_task::~enc_task()
{
   if (buffer_id != VA_INVALID_ID)
      vaUnmapBuffer(encoder->dpy, buffer_id);

   encoder->release_buffer(buffer_id);
}

bool enc_task::wait(uint64_t timeout_ns)
{
   VAStatus status = vaSyncBuffer(encoder->dpy, buffer_id, timeout_ns);
   if (status == VA_STATUS_ERROR_TIMEDOUT || !va_check(status, "vaSyncBuffer"))
      return false;

   status = vaMapBuffer2(encoder->dpy, buffer_id, reinterpret_cast<void**>(&coded_buf), VA_MAPBUFFER_FLAG_READ);
   return va_check(status, "vaMapBuffer2");
}

bool enc_task::get_bitstream(uint32_t *size, uint8_t **data)
{
   if (!coded_buf)
      return false;

   *size = coded_buf->size;
   *data = reinterpret_cast<uint8_t *>(coded_buf->buf);
   coded_buf = static_cast<VACodedBufferSegment *>(coded_buf->next);
   return true;
}
