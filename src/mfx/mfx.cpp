#define ONEVPL_EXPERIMENTAL
#include <vpl/mfxvideo.h>

#include <iostream>

#include "caps.h"
#include "session.h"

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
   delete Session::from_mfx(session);
   return MFX_ERR_NONE;
}

mfxStatus MFXJoinSession(mfxSession session, mfxSession child)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_GetHandle(mfxSession session, mfxHandleType type, mfxHDL *hdl)
{
   return Session::from_mfx(session)->GetHandle(type, hdl);
}

mfxStatus MFXQueryIMPL(mfxSession, mfxIMPL *impl)
{
   *impl = MFX_IMPL_HARDWARE;
   return MFX_ERR_NONE;
}

mfxStatus MFXQueryVersion(mfxSession, mfxVersion *version)
{
   *version = { { VERSION_MINOR, VERSION_MAJOR } };
   return MFX_ERR_NONE;
}

mfxStatus MFXVideoCORE_SetFrameAllocator(mfxSession session, mfxFrameAllocator *allocator)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoCORE_SetHandle(mfxSession session, mfxHandleType type, mfxHDL hdl)
{
   return Session::from_mfx(session)->SetHandle(type, hdl);
}

mfxStatus MFXVideoCORE_SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait)
{
   return Session::from_mfx(session)->SyncOperation(syncp, wait);
}

mfxStatus MFXVideoENCODE_Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
   return Session::from_mfx(session)->Query(in, out);
}

mfxStatus MFXVideoENCODE_QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
   return Session::from_mfx(session)->QueryIOSurf(par, request);
}

mfxStatus MFXVideoENCODE_Init(mfxSession session, mfxVideoParam *par)
{
   return Session::from_mfx(session)->Init(par);
}

mfxStatus MFXVideoENCODE_Reset(mfxSession session, mfxVideoParam *par)
{
   return Session::from_mfx(session)->Reset(par);
}

mfxStatus MFXVideoENCODE_Close(mfxSession session)
{
   return Session::from_mfx(session)->Close();
}

mfxStatus MFXVideoENCODE_GetVideoParam(mfxSession session, mfxVideoParam *par)
{
   return Session::from_mfx(session)->GetVideoParam(par);
}

mfxStatus MFXVideoENCODE_GetEncodeStat(mfxSession session, mfxEncodeStat *stat)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_UNKNOWN;
}

mfxStatus MFXVideoENCODE_EncodeFrameAsync(mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
   return Session::from_mfx(session)->EncodeFrameAsync(ctrl, surface, bs, syncp);
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
   return Session::from_mfx(session)->QueryPlatform(platform);
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
   Session *s = new Session(par);
   *session = s->to_mfx();
   return MFX_ERR_NONE;
}

}
