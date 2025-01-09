#include "session.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <xf86drm.h>

#include "../surface.h"

class SyncPoint
{
public:
   mfxBitstream *bs = nullptr;
   struct enc_task *task = nullptr;
};


Session::Session(const mfxInitializationParam &par)
{
   dev_path = "/dev/dri/renderD" + std::to_string(par.VendorImplID + 128);
}

Session::~Session()
{
   if (dev)
      enc_dev_destroy(dev);
}

mfxStatus Session::SetHandle(mfxHandleType type, mfxHDL hdl)
{
   switch (type) {
   case MFX_HANDLE_VA_DISPLAY: {
      if (dpy)
         break;
      struct enc_dev_params dev_params = {};
      dev_params.va_display = hdl;
      dev = enc_dev_create(&dev_params);
      if (!dev)
         break;
      dpy = hdl;
      return MFX_ERR_NONE;
   }
   default:
      break;
   }
   return MFX_ERR_UNDEFINED_BEHAVIOR;
}

mfxStatus Session::GetHandle(mfxHandleType type, mfxHDL *hdl)
{
   switch (type) {
   case MFX_HANDLE_VA_DISPLAY:
      *hdl = dpy;
      return MFX_ERR_NONE;
   default:
      break;
   }
   return MFX_ERR_UNDEFINED_BEHAVIOR;
}

mfxStatus Session::GetVideoParam(mfxVideoParam *par)
{
   *par = param;
   return MFX_ERR_NONE;
}

mfxStatus Session::Query(mfxVideoParam *in, mfxVideoParam *out)
{
   *out = *in;
   return MFX_ERR_NONE;
}

mfxStatus Session::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request)
{
   request->NumFrameSuggested = par->AsyncDepth + 4;
   request->Type |= MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY;
   request->Info = par->mfx.FrameInfo;
   return MFX_ERR_NONE;
}

mfxStatus Session::QueryPlatform(mfxPlatform *platform)
{
   memset(platform, 0, sizeof(*platform));
   platform->MediaAdapterType = MFX_MEDIA_UNKNOWN;
   int fd = open(dev_path.c_str(), O_RDWR);
   if (fd >= 0) {
      drmDevicePtr dev = nullptr;
      drmGetDevice(fd, &dev);
      if (dev)
         platform->DeviceId = dev->deviceinfo.pci->device_id;
      drmFreeDevice(&dev);
      close(fd);
   }
   return MFX_ERR_NONE;
}

mfxStatus Session::Init(mfxVideoParam *par)
{
   param = *par;

   struct enc_rate_control_params rc = {
      .frame_rate = 30.0,
      .bit_rate = 10000,
      .vbv_buffer_size = 1000,
   };

   struct enc_encoder_params encoder_params = {
      .dev = dev,
      .codec = ENC_CODEC_H264,
      .width = param.mfx.FrameInfo.CropW,
      .height = param.mfx.FrameInfo.CropH,
      .bit_depth = 8,
      .num_refs = 1,
      .gop_size = param.mfx.GopRefDist,
      .num_slices = 1,
      .rc_mode = ENC_RATE_CONTROL_MODE_CQP,
      .num_rc_layers = 1,
      .rc_params = &rc,
      .h264 = {
         .profile = ENC_H264_PROFILE_HIGH,
         .level = 52,
      },
   };

   enc = enc_encoder_create(&encoder_params);
   if (!enc)
      return MFX_ERR_DEVICE_FAILED;

   return MFX_ERR_NONE;
}

mfxStatus Session::Reset(mfxVideoParam *par)
{
   return MFX_ERR_NONE;
}

mfxStatus Session::Close()
{
   if (enc) {
      enc_encoder_destroy(enc);
      enc = nullptr;
   }
   return MFX_ERR_NONE;
}

mfxStatus Session::EncodeFrameAsync(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
   if (!surface)
      return MFX_ERR_MORE_DATA;

   struct enc_surface_params surface_params = {
      .dev = dev,
      .format = ENC_FORMAT_NV12,
      .width = surface->Info.Width,
      .height = surface->Info.Height,
   };
   struct enc_surface *surf = enc_surface_create(&surface_params);

   enc_surface *su = reinterpret_cast<enc_surface*>(surf);

   VAImage image;
   vaDeriveImage(dpy, su->surface_id, &image);

   uint8_t *data;
   vaMapBuffer2(dpy, image.buf, reinterpret_cast<void**>(&data), VA_MAPBUFFER_FLAG_WRITE);

   for (uint32_t i = 0; i < surface->Info.Height; i++)
      memcpy(data + i * image.pitches[0], surface->Data.Y + i * surface->Data.Pitch, surface->Data.Pitch);

   for (uint32_t i = 0; i < surface->Info.Height / 2; i++)
      memcpy(data + image.offsets[1] + i * image.pitches[1], surface->Data.UV + i * surface->Data.Pitch, surface->Data.Pitch);

   vaUnmapBuffer(dpy, image.buf);
   vaDestroyImage(dpy, image.image_id);

   // TODO COPY

   struct enc_frame_feedback feedback;
   struct enc_frame_params frame_params = {
      .feedback = &feedback,
   };

   frame_params.surface = surf;
   frame_params.qp = 20;

   struct enc_task *task = enc_encoder_encode_frame(enc, &frame_params);

   enc_surface_destroy(surf);

   if (!task)
      return MFX_ERR_DEVICE_LOST;

   SyncPoint *sp = new SyncPoint;
   sp->task = task;
   sp->bs = bs;

   *syncp = reinterpret_cast<mfxSyncPoint>(sp);

   return MFX_ERR_NONE;
}

mfxStatus Session::SyncOperation(mfxSyncPoint syncp, mfxU32 wait)
{
   SyncPoint *sp = reinterpret_cast<SyncPoint*>(syncp);

   if (!enc_task_wait(sp->task, wait * 1000))
      return MFX_WRN_IN_EXECUTION;

   uint32_t size = 0;
   uint8_t *data = NULL;

   uint8_t *out = sp->bs->Data;
   sp->bs->DataLength = 0;

   while (enc_task_get_bitstream(sp->task, &size, &data)) {
      memcpy(out, data, size);
      out += size;
      sp->bs->DataLength += size;
   }

   enc_task_destroy(sp->task);
   sp->task = nullptr;
   delete sp;

   return MFX_ERR_NONE;
}
