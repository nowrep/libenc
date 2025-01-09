#pragma once

#include <va/va.h>
#include <vpl/mfxvideo.h>

#include <enc/enc.h>
#include "../dev.h"
#include "../encoder.h"

#include <string>
#include <memory>

class Session
{
public:
   Session(const mfxInitializationParam &par);
   ~Session();

   VADisplay dpy = nullptr;
   std::string dev_path;

   mfxVideoParam param = {};

   std::unique_ptr<enc_dev> dev;
   std::unique_ptr<enc_encoder> enc;

   mfxSession to_mfx()
   {
      return reinterpret_cast<mfxSession>(this);
   }

   static Session *from_mfx(mfxSession session)
   {
      return reinterpret_cast<Session *>(session);
   }

   mfxStatus SetHandle(mfxHandleType type, mfxHDL hdl);
   mfxStatus GetHandle(mfxHandleType type, mfxHDL *hdl);
   mfxStatus GetVideoParam(mfxVideoParam *par);
   mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out);
   mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request);
   mfxStatus QueryPlatform(mfxPlatform *platform);
   mfxStatus Init(mfxVideoParam *par);
   mfxStatus Reset(mfxVideoParam *par);
   mfxStatus Close();
   mfxStatus EncodeFrameAsync(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp);
   mfxStatus SyncOperation(mfxSyncPoint syncp, mfxU32 wait);
};
