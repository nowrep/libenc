#define ONEVPL_EXPERIMENTAL
#include <vpl/mfxvideo.h>
#include <va/va.h>
#include <enc/enc.h>
#include "../surface.h"

#include <memory.h>
#include <iostream>

#include "caps.h"

class Session
{
public:
   VADisplay dpy = nullptr;
   mfxVideoParam param;

   struct enc_dev *dev = nullptr;
   struct enc_encoder *enc = nullptr;
};

class SyncPoint
{
public:
   mfxBitstream *bs = nullptr;
   struct enc_task *task = nullptr;
};

extern "C"
{

// 1.0

mfxStatus MFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXClose(mfxSession session)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXJoinSession(mfxSession session, mfxSession child)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_GetHandle(mfxSession session, mfxHandleType type, mfxHDL *hdl)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXQueryIMPL(mfxSession session, mfxIMPL *impl)
{
   std::cout << __FUNCTION__ << std::endl;
   *impl = MFX_IMPL_HARDWARE;
   return MFX_ERR_NONE;
}

mfxStatus MFXQueryVersion(mfxSession session, mfxVersion *version)
{
   std::cout << __FUNCTION__ << std::endl;
   *version = mfxVersion {
      .Minor = 0,
      .Major = 2,
   };
   return MFX_ERR_NONE;
}

mfxStatus MFXVideoCORE_SetFrameAllocator(mfxSession session, mfxFrameAllocator *allocator)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_SetHandle(mfxSession session, mfxHandleType type, mfxHDL hdl)
{
   std::cout << __FUNCTION__ << " " << type << std::endl;

   Session *s = reinterpret_cast<Session*>(session);

   switch (type) {
   case MFX_HANDLE_VA_DISPLAY: {
      struct enc_dev_params dev_params = {
         .va_display = hdl,
      };
      s->dpy = hdl;
      s->dev = enc_dev_create(&dev_params);
      if (!s->dev)
         return MFX_ERR_DEVICE_FAILED;
      break;
                               }
   case MFX_HANDLE_VA_CONFIG_ID:
   case MFX_HANDLE_VA_CONTEXT_ID:
   default:
      return MFX_ERR_UNKNOWN;
   }

   return MFX_ERR_NONE;
}

mfxStatus MFXVideoCORE_SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait)
{
   std::cout << __FUNCTION__ << std::endl;

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
   // delete sp;

   return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
   std::cout << __FUNCTION__ << std::endl;

   request->NumFrameSuggested = par->AsyncDepth + 4;
   request->Type |= MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY;
   request->Info = par->mfx.FrameInfo;

   return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODE_Init(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;

   Session *s = reinterpret_cast<Session*>(session);
   s->param = *par;

   struct enc_rate_control_params rc = {
      .frame_rate = 30.0,
      .bit_rate = 10000,
      .vbv_buffer_size = 1000,
   };

   struct enc_encoder_params encoder_params = {
      .dev = s->dev,
      .codec = ENC_CODEC_H264,
      .width = s->param.mfx.FrameInfo.CropW,
      .height = s->param.mfx.FrameInfo.CropH,
      .bit_depth = 8,
      .num_refs = 1,
      .gop_size = s->param.mfx.GopRefDist,
      .num_slices = 1,
      .rc_mode = ENC_RATE_CONTROL_MODE_CQP,
      .num_rc_layers = 1,
      .rc_params = &rc,
      .h264 = {
         .profile = ENC_H264_PROFILE_HIGH,
         .level = 52,
      },
   };

   s->enc = enc_encoder_create(&encoder_params);
   if (!s->enc)
      return MFX_ERR_DEVICE_FAILED;

   return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODE_Reset(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;

   Session *s = reinterpret_cast<Session*>(session);

   if (s->enc) {
      enc_encoder_destroy(s->enc);
      s->enc = nullptr;
   }

   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoENCODE_Close(mfxSession session)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODE_GetVideoParam(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;

   Session *s = reinterpret_cast<Session*>(session);
   *par = s->param;

   return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODE_GetEncodeStat(mfxSession session, mfxEncodeStat *stat)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoENCODE_EncodeFrameAsync(mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
   std::cout << __FUNCTION__ << std::endl;

   if (!surface) {
      printf("flush\n");
      return MFX_ERR_MORE_DATA;
   }


   Session *s = reinterpret_cast<Session*>(session);

   struct enc_surface_params surface_params = {
      .dev = s->dev,
      .format = ENC_FORMAT_NV12,
      .width = surface->Info.Width,
      .height = surface->Info.Height,
   };
   struct enc_surface *surf = enc_surface_create(&surface_params);

   enc_surface *su = reinterpret_cast<enc_surface*>(surf);

   VAImage image;
   vaDeriveImage(s->dpy, su->surface_id, &image);

   uint8_t *data;
   vaMapBuffer2(s->dpy, image.buf, reinterpret_cast<void**>(&data), VA_MAPBUFFER_FLAG_WRITE);

   for (uint32_t i = 0; i < surface->Info.Height; i++)
      memcpy(data + i * image.pitches[0], surface->Data.Y + i * surface->Data.Pitch, surface->Data.Pitch);

   for (uint32_t i = 0; i < surface->Info.Height / 2; i++)
      memcpy(data + image.offsets[1] + i * image.pitches[1], surface->Data.UV + i * surface->Data.Pitch, surface->Data.Pitch);

   vaUnmapBuffer(s->dpy, image.buf);
   vaDestroyImage(s->dpy, image.image_id);

   // TODO COPY

   struct enc_frame_feedback feedback;
   struct enc_frame_params frame_params = {
      .feedback = &feedback,
   };

   frame_params.surface = surf;
   frame_params.qp = 20;

   struct enc_task *task = enc_encoder_encode_frame(s->enc, &frame_params);

   enc_surface_destroy(surf);

   if (!task)
      return MFX_ERR_DEVICE_LOST;

   SyncPoint *sp = new SyncPoint;
   sp->task = task;
   sp->bs = bs;

   *syncp = reinterpret_cast<mfxSyncPoint>(sp);

   return MFX_ERR_NONE;
}

mfxStatus MFXVideoDECODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_DecodeHeader(mfxSession session, mfxBitstream *bs, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_Init(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_Reset(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_Close(mfxSession session)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_GetVideoParam(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_GetDecodeStat(mfxSession session, mfxDecodeStat *stat)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_SetSkipMode(mfxSession session, mfxSkipMode mode)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_GetPayload(mfxSession session, mfxU64 *ts, mfxPayload *payload)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoDECODE_DecodeFrameAsync(mfxSession session, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest request[2])
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_Init(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_Reset(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_Close(mfxSession session)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_GetVideoParam(mfxSession session, mfxVideoParam *par)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_GetVPPStat(mfxSession session, mfxVPPStat *stat)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoVPP_RunFrameVPPAsync(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXDisjoinSession(mfxSession session)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXSetPriority(mfxSession session, mfxPriority priority)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXGetPriority(mfxSession session, mfxPriority *priority)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_QueryPlatform(mfxSession session, mfxPlatform* platform)
{
   std::cout << __FUNCTION__ << std::endl;

   platform->MediaAdapterType = MFX_MEDIA_DISCRETE;

   return MFX_ERR_NONE;
}

// 2.0
mfxHDL *MFXQueryImplsDescription(mfxImplCapsDeliveryFormat format, mfxU32 *num_impls)
{
   return QueryImplsDescription(format, num_impls);
}

mfxStatus MFXReleaseImplDescription(mfxHDL hdl)
{
   return ReleaseImplDescription(hdl);
}

mfxStatus MFXMemory_GetSurfaceForVPP(mfxSession session, mfxFrameSurface1** surface)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXMemory_GetSurfaceForEncode(mfxSession session, mfxFrameSurface1** surface)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXMemory_GetSurfaceForDecode(mfxSession session, mfxFrameSurface1** surface)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXInitialize(mfxInitializationParam par, mfxSession *session)
{
   std::cout << __FUNCTION__ << std::endl;

   *session = reinterpret_cast<mfxSession>(new Session);
   return MFX_ERR_NONE;
}

}
