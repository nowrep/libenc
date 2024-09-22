#include "util.h"
#include "dev.h"
#include "task.h"
#include "surface.h"
#include "encoder_h264.h"
#include "encoder_hevc.h"

#include <iostream>
#include <memory>

struct enc_dev *enc_dev_create(const struct enc_dev_params *params)
{
   auto dev = std::make_unique<enc_dev>();
   if (!dev->create(params))
      return {};

   return dev.release();
}

void enc_dev_destroy(struct enc_dev *dev)
{
   vaTerminate(dev->dpy);
   delete dev;
}

struct enc_surface *enc_surface_create(const struct enc_surface_params *params)
{
   auto surface = std::make_unique<enc_surface>();
   if (!surface->create(params))
      return {};

   return surface.release();
}

void enc_surface_destroy(struct enc_surface *surface)
{
   delete surface;
}

bool enc_surface_export_dmabuf(struct enc_surface *surface, struct enc_dmabuf *dmabuf)
{
   return surface->export_dmabuf(dmabuf);
}

struct enc_encoder *enc_encoder_create(const struct enc_encoder_params *params)
{
   std::unique_ptr<enc_encoder> encoder;

   switch (params->codec) {
   case ENC_CODEC_H264:
      encoder = std::make_unique<encoder_h264>();
      break;
   case ENC_CODEC_HEVC:
      encoder = std::make_unique<encoder_hevc>();
      break;
   default:
      std::cerr << "Invalid codec " << params->codec << std::endl;
      return {};
   }

   if (!encoder->create(params))
       return {};

   return encoder.release();
}

void enc_encoder_destroy(struct enc_encoder *encoder)
{
   delete encoder;
}

struct enc_task *enc_encoder_encode_frame(struct enc_encoder *encoder, const struct enc_frame_params *params)
{
   return encoder->encode_frame(params);
}

void enc_task_destroy(struct enc_task *task)
{
   delete task;
}

bool enc_task_wait(struct enc_task *task, uint64_t timeout_ns)
{
   return task->wait(timeout_ns);
}

bool enc_task_get_bitstream(struct enc_task *task, uint32_t *size, uint8_t **data)
{
   return task->get_bitstream(size, data);
}
