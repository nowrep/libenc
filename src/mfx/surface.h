#pragma once

#include "session.h"

class Surface : public mfxFrameSurface1
{
public:
   Surface(Session *session);
   ~Surface();

   mfxFrameSurface1 *to_mfx()
   {
      return static_cast<mfxFrameSurface1 *>(this);
   }

   static Surface *from_mfx(mfxFrameSurface1 *surface)
   {
      return static_cast<Surface *>(surface);
   }

   bool create();

   static mfxU32 Context;
   static mfxStatus AddRef(mfxFrameSurface1 *surface);
   static mfxStatus Release(mfxFrameSurface1 *surface);
   static mfxStatus GetRefCounter(mfxFrameSurface1 *surface, mfxU32 *counter);
   static mfxStatus Map(mfxFrameSurface1 *surface, mfxU32 flags);
   static mfxStatus Unmap(mfxFrameSurface1 *surface);
   static mfxStatus GetNativeHandle(mfxFrameSurface1 *surface, mfxHDL *resource, mfxResourceType *resource_type);
   static mfxStatus GetDeviceHandle(mfxFrameSurface1 *surface, mfxHDL *device_handle, mfxHandleType *device_type);
   static mfxStatus Synchronize(mfxFrameSurface1 *surface, mfxU32 wait);
   static void OnComplete(mfxStatus sts);
   static mfxStatus QueryInterface(mfxFrameSurface1 *surface, mfxGUID guid, mfxHDL *iface);

   Session *session;
   mfxU32 ref = 1;
   VASurfaceID surface_id = VA_INVALID_SURFACE;
   VAImage image = {};
};
