#include "session.h"
#include "surface.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <xf86drm.h>

#include "../surface.h"
#include "../encoder_h264.h"
#include "../encoder_hevc.h"
#include "../encoder_av1.h"

template <typename T>
static T *find_ext_buf(mfxVideoParam *param, mfxU32 id)
{
   for (auto i = 0; i < param->NumExtParam; i++) {
      if (param->ExtParam[i]->BufferId == id)
         return reinterpret_cast<T *>(param->ExtParam[i]);
   }
   return nullptr;
}

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
}

mfxStatus Session::SetHandle(mfxHandleType type, mfxHDL hdl)
{
   switch (type) {
   case MFX_HANDLE_VA_DISPLAY: {
      if (dpy)
         break;
      struct enc_dev_params params = {};
      params.va_display = hdl;
      dev = std::make_unique<enc_dev>();
      if (!dev->create(&params)) {
         dev = {};
         break;
      }
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
   case MFX_HANDLE_MEMORY_INTERFACE:
      if (!mem) {
         mem = std::make_unique<mfxMemoryInterface>();
         mem->Context = this;
         mem->Version.Version = MFX_MEMORYINTERFACE_VERSION;
         mem->ImportFrameSurface = Session::ImportFrameSurface;
      }
      *hdl = mem.get();
      return MFX_ERR_NONE;
   default:
      break;
   }
   return MFX_ERR_UNDEFINED_BEHAVIOR;
}

mfxStatus Session::GetVideoParam(mfxVideoParam *par)
{
   if (!enc)
      return MFX_ERR_UNDEFINED_BEHAVIOR;

   if (auto sps_pps = find_ext_buf<mfxExtCodingOptionSPSPPS>(par, MFX_EXTBUFF_CODING_OPTION_SPSPPS)) {
      sps_pps->SPSBufSize = enc->write_sps(sps_pps->SPSBuffer, sps_pps->SPSBufSize);
      sps_pps->PPSBufSize = enc->write_pps(sps_pps->PPSBuffer, sps_pps->PPSBufSize);
   }

   if (auto vps = find_ext_buf<mfxExtCodingOptionVPS>(par, MFX_EXTBUFF_CODING_OPTION_VPS))
      vps->VPSBufSize = enc->write_vps(vps->VPSBuffer, vps->VPSBufSize);

   *par = param;
   return MFX_ERR_NONE;
}

mfxStatus Session::Query(mfxVideoParam *in, mfxVideoParam *out)
{
   *out = *in;
   out->AllocId = ++alloc_id;
   return MFX_ERR_NONE;
}

mfxStatus Session::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request)
{
   memset(request, 0, sizeof(*request));
   request->AllocId = par->AllocId;
   request->Info = par->mfx.FrameInfo;
   request->Type = MFX_MEMTYPE_FROM_ENCODE;
   if (par->IOPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY)
      request->Type |= MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET;
   else if (par->IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
      request->Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
   request->NumFrameMin = par->mfx.NumRefFrame + par->AsyncDepth + 1;
   request->NumFrameSuggested = request->NumFrameMin;
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
   if (enc)
      return MFX_ERR_UNDEFINED_BEHAVIOR;

   param = *par;

   if (!param.mfx.BRCParamMultiplier)
      param.mfx.BRCParamMultiplier = 1;
   if (!param.mfx.BufferSizeInKB)
      param.mfx.BufferSizeInKB = 1024;

   if (!dev) {
      struct enc_dev_params params = {};
      params.device_path = dev_path.c_str();
      dev = std::make_unique<enc_dev>();
      if (!dev->create(&params)) {
         dev = {};
         return MFX_ERR_DEVICE_LOST;
      }
      dpy = dev->dpy;
   }

   if (alloc) {
      mfxFrameAllocRequest request = {};
      QueryIOSurf(par, &request);
      if (alloc->Alloc(alloc->pthis, &request, &alloc_response) != MFX_ERR_NONE)
         return MFX_ERR_MEMORY_ALLOC;
   }

   struct enc_rate_control_params rc = {};
   rc.frame_rate = param.mfx.FrameInfo.FrameRateExtN / static_cast<float>(param.mfx.FrameInfo.FrameRateExtN);
   rc.bit_rate = param.mfx.TargetKbps * 1000;
   rc.peak_bit_rate = param.mfx.TargetKbps * 1000;
   rc.vbv_buffer_size = param.mfx.BufferSizeInKB * 1024 * 8;
   rc.vbv_initial_fullness = param.mfx.InitialDelayInKB * 1024 * 8;
   rc.qvbr_quality = param.mfx.Quality;

   if (!rc.bit_rate)
      rc.bit_rate = 10000;
   if (!rc.peak_bit_rate)
      rc.peak_bit_rate = rc.bit_rate;
   if (!rc.vbv_buffer_size)
      rc.vbv_buffer_size = rc.bit_rate;
   if (!rc.vbv_initial_fullness)
      rc.vbv_initial_fullness = rc.vbv_buffer_size;

   struct enc_encoder_params params = {};
   params.dev = dev.get();
   params.width = param.mfx.FrameInfo.CropW;
   params.height = param.mfx.FrameInfo.CropH;
   params.bit_depth = 8;
   params.num_refs = param.mfx.NumRefFrame;
   params.gop_size = param.mfx.GopRefDist;
   params.num_slices = param.mfx.NumSlice;
   params.num_rc_layers = 1;
   params.rc_params = &rc;

   switch (param.mfx.RateControlMethod) {
   case MFX_RATECONTROL_CBR:
      params.rc_mode = ENC_RATE_CONTROL_MODE_CBR;
      break;
   case MFX_RATECONTROL_VBR:
      params.rc_mode = ENC_RATE_CONTROL_MODE_VBR;
      break;
   case MFX_RATECONTROL_QVBR:
      params.rc_mode = ENC_RATE_CONTROL_MODE_QVBR;
      break;
   default:
      params.rc_mode = ENC_RATE_CONTROL_MODE_CQP;
      break;
   }

   switch (param.mfx.CodecId) {
   case MFX_CODEC_AVC:
      enc = std::make_unique<encoder_h264>();
      params.h264.level = param.mfx.CodecLevel ? param.mfx.CodecLevel : 52;
      switch (param.mfx.CodecProfile) {
      case MFX_PROFILE_AVC_CONSTRAINED_BASELINE:
         params.h264.profile = ENC_H264_PROFILE_CONSTRAINED_BASELINE;
         break;
      case MFX_PROFILE_AVC_MAIN:
         params.h264.profile = ENC_H264_PROFILE_MAIN;
         break;
      default:
      case MFX_PROFILE_AVC_HIGH:
         params.h264.profile = ENC_H264_PROFILE_HIGH;
         break;
      }
      break;
   case MFX_CODEC_HEVC:
      enc = std::make_unique<encoder_hevc>();
      params.hevc.level = param.mfx.CodecLevel ? param.mfx.CodecLevel : 85;
      switch (param.mfx.CodecProfile) {
      default:
      case MFX_PROFILE_HEVC_MAIN:
         params.hevc.profile = ENC_HEVC_PROFILE_MAIN;
         break;
      case MFX_PROFILE_HEVC_MAIN10:
         params.hevc.profile = ENC_HEVC_PROFILE_MAIN_10;
         break;
      }
      break;
   case MFX_CODEC_AV1:
      enc = std::make_unique<encoder_av1>();
      params.av1.level = param.mfx.CodecLevel ? param.mfx.CodecLevel : 60;
      params.av1.profile = ENC_AV1_PROFILE_0;
      break;
   default:
      break;
   }

   if (!enc->create(&params)) {
      enc = {};
      return MFX_ERR_DEVICE_FAILED;
   }

   return MFX_ERR_NONE;
}

mfxStatus Session::Reset(mfxVideoParam *par)
{
   param = *par;
   return MFX_ERR_NONE;
}

mfxStatus Session::Close()
{
   if (alloc && alloc->Free(alloc->pthis, &alloc_response) != MFX_ERR_NONE)
      return MFX_ERR_MEMORY_ALLOC;
   alloc = nullptr;
   enc = {};
   return MFX_ERR_NONE;
}

mfxStatus Session::EncodeFrameAsync(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
   if (!surface)
      return MFX_ERR_MORE_DATA;

   mfxFrameSurface1 *surf = surface;

   if (!alloc) {
      if (surface->Data.Y) {
         if (GetSurfaceForEncode(&surf) != MFX_ERR_NONE)
            return MFX_ERR_DEVICE_LOST;
         if (surf->FrameInterface->Map(surf, MFX_MAP_WRITE) != MFX_ERR_NONE)
            return MFX_ERR_DEVICE_LOST;
         for (mfxU32 i = 0; i < surface->Info.Height; i++)
            memcpy(surf->Data.Y + i * surf->Data.Pitch, surface->Data.Y + i * surface->Data.Pitch, surface->Data.Pitch);
         if (surface->Info.FourCC == MFX_FOURCC_NV12 || surface->Info.FourCC == MFX_FOURCC_P010) {
            for (uint32_t i = 0; i < surface->Info.Height / 2; i++)
               memcpy(surf->Data.UV + i * surf->Data.Pitch, surface->Data.UV + i * surface->Data.Pitch, surface->Data.Pitch);
         }
         surf->FrameInterface->Unmap(surf);
      } else if (surface->FrameInterface && surface->FrameInterface->Context == &Surface::Context) {
         surf = surface;
         surf->FrameInterface->AddRef(surf);
      } else {
         return MFX_ERR_DEVICE_LOST;
      }
   }

   struct enc_surface surf_in = {};
   surf_in.surface_id = VA_INVALID_SURFACE;

   if (alloc) {
      mfxHDLPair pair = {};
      if (alloc->GetHDL(alloc->pthis, surf->Data.MemId, reinterpret_cast<mfxHDL *>(&pair)) == MFX_ERR_NONE) {
         VASurfaceID *id = reinterpret_cast<VASurfaceID *>(pair.first);
         if (id)
            surf_in.surface_id = *id;
      }
   } else if (surf->FrameInterface) {
      surf->FrameInterface->GetNativeHandle(surf, reinterpret_cast<mfxHDL *>(&surf_in.surface_id), nullptr);
   }

   if (surf_in.surface_id == VA_INVALID_SURFACE)
      return MFX_ERR_DEVICE_LOST;

   struct enc_frame_feedback fb = {};
   struct enc_frame_params frame_params = {};
   frame_params.feedback = &fb;
   frame_params.surface = &surf_in;
   frame_params.qp = param.mfx.QPI;

   if (ctrl) {
      if (ctrl->QP)
         frame_params.qp = ctrl->QP;

      if (ctrl->FrameType) {
         if (ctrl->FrameType & MFX_FRAMETYPE_IDR) {
            frame_params.frame_type = ENC_FRAME_TYPE_IDR;
         } else if (ctrl->FrameType & MFX_FRAMETYPE_I) {
            frame_params.frame_type = ENC_FRAME_TYPE_I;
         } else if (ctrl->FrameType & MFX_FRAMETYPE_P) {
            frame_params.frame_type = ENC_FRAME_TYPE_P;
            frame_params.not_referenced = !(ctrl->FrameType & MFX_FRAMETYPE_REF);
         }
      }
   }

   struct enc_task *task = enc->encode_frame(&frame_params);

   if (!alloc && surf->FrameInterface)
      surf->FrameInterface->Release(surf);

   if (!task)
      return MFX_ERR_DEVICE_LOST;

   SyncPoint *sp = new SyncPoint;
   sp->task = task;
   sp->bs = bs;
   sp->bs->TimeStamp = surface->Data.TimeStamp;
   sp->bs->DecodeTimeStamp = surface->Data.TimeStamp;
   sp->bs->PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
   sp->bs->DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;

   switch (fb.frame_type) {
   case ENC_FRAME_TYPE_I:
      sp->bs->FrameType = MFX_FRAMETYPE_I;
      break;
   case ENC_FRAME_TYPE_IDR:
      sp->bs->FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR;
      break;
   case ENC_FRAME_TYPE_P:
      sp->bs->FrameType = MFX_FRAMETYPE_P;
      break;
   default:
      break;
   }

   if (fb.referenced)
      sp->bs->FrameType |= MFX_FRAMETYPE_REF;

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

   mfxStatus ret = MFX_ERR_NONE;

   while (enc_task_get_bitstream(sp->task, &size, &data)) {
      if (sp->bs->DataLength + size < sp->bs->MaxLength) {
         memcpy(out, data, size);
         out += size;
         sp->bs->DataLength += size;
      } else {
         ret = MFX_ERR_NOT_ENOUGH_BUFFER;
      }
   }

   enc_task_destroy(sp->task);
   sp->task = nullptr;
   delete sp;

   return ret;
}

mfxStatus Session::GetSurfaceForEncode(mfxFrameSurface1 **surface)
{
   auto s = std::make_unique<Surface>(this);
   if (!s->create())
      return MFX_ERR_MEMORY_ALLOC;
   *surface = s.release();
   return MFX_ERR_NONE;
}

mfxStatus Session::SetFrameAllocator(mfxFrameAllocator *allocator)
{
   alloc = allocator;
   return MFX_ERR_NONE;
}

mfxStatus Session::ImportFrameSurface(struct mfxMemoryInterface *memory_interface, mfxSurfaceComponent surf_component, mfxSurfaceHeader *external_surface, mfxFrameSurface1 **imported_surface)
{
   if (surf_component != MFX_SURFACE_COMPONENT_ENCODE)
      return MFX_ERR_UNSUPPORTED;
   auto session = reinterpret_cast<Session *>(memory_interface->Context);
   auto vaapi = reinterpret_cast<mfxSurfaceVAAPI *>(external_surface);
   if (vaapi->vaDisplay != session->dpy)
      return MFX_ERR_INVALID_HANDLE;
   auto s = std::make_unique<Surface>(session);
   s->surface_id = vaapi->vaSurfaceID;
   *imported_surface = s.release();
   return MFX_ERR_NONE;
}
