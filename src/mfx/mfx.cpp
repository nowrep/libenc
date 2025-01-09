#define ONEVPL_EXPERIMENTAL
#include <vpl/mfxvideo.h>

#include <iostream>

#include "caps.h"
#include "session.h"

extern "C"
{

// 1.0

mfxStatus MFXInit(mfxIMPL, mfxVersion *, mfxSession *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXInitEx(mfxInitParam, mfxSession *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXClose(mfxSession session)
{
   delete Session::from_mfx(session);
   return MFX_ERR_NONE;
}

mfxStatus MFXJoinSession(mfxSession, mfxSession)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
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

mfxStatus MFXVideoCORE_SetFrameAllocator(mfxSession, mfxFrameAllocator *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
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

mfxStatus MFXVideoENCODE_GetEncodeStat(mfxSession, mfxEncodeStat *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoENCODE_EncodeFrameAsync(mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
   return Session::from_mfx(session)->EncodeFrameAsync(ctrl, surface, bs, syncp);
}

mfxStatus MFXVideoDECODE_Query(mfxSession, mfxVideoParam *, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_DecodeHeader(mfxSession, mfxBitstream *, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_QueryIOSurf(mfxSession, mfxVideoParam *, mfxFrameAllocRequest *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_Init(mfxSession, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_Reset(mfxSession, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_Close(mfxSession)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_GetVideoParam(mfxSession, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_GetDecodeStat(mfxSession, mfxDecodeStat *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_SetSkipMode(mfxSession, mfxSkipMode)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_GetPayload(mfxSession, mfxU64 *, mfxPayload *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoDECODE_DecodeFrameAsync(mfxSession, mfxBitstream *, mfxFrameSurface1 *, mfxFrameSurface1 **, mfxSyncPoint *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_Query(mfxSession, mfxVideoParam *, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_QueryIOSurf(mfxSession, mfxVideoParam *, mfxFrameAllocRequest[2])
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_Init(mfxSession, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_Reset(mfxSession, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_Close(mfxSession)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_GetVideoParam(mfxSession, mfxVideoParam *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_GetVPPStat(mfxSession, mfxVPPStat *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXVideoVPP_RunFrameVPPAsync(mfxSession, mfxFrameSurface1 *, mfxFrameSurface1 *, mfxExtVppAuxData *, mfxSyncPoint *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXDisjoinSession(mfxSession)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXSetPriority(mfxSession, mfxPriority)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXGetPriority(mfxSession, mfxPriority *)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
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

mfxStatus MFXMemory_GetSurfaceForVPP(mfxSession, mfxFrameSurface1 **)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXMemory_GetSurfaceForEncode(mfxSession, mfxFrameSurface1 **)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXMemory_GetSurfaceForDecode(mfxSession, mfxFrameSurface1 **)
{
   std::cout << __FUNCTION__ << std::endl;
   return MFX_ERR_NOT_IMPLEMENTED;
}

mfxStatus MFXInitialize(mfxInitializationParam par, mfxSession *session)
{
   Session *s = new Session(par);
   *session = s->to_mfx();
   return MFX_ERR_NONE;
}

}
