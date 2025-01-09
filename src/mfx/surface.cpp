#include "surface.h"

mfxU32 Surface::Context = 0xAB;

Surface::Surface(Session *session)
   : session(session)
{
   *to_mfx() = {};
   Version.Version = MFX_FRAMESURFACE1_VERSION;
   Info = session->param.mfx.FrameInfo;
   FrameInterface = new mfxFrameSurfaceInterface();
   FrameInterface->Context = &Context;
   FrameInterface->Version.Version = MFX_FRAMESURFACEINTERFACE_VERSION;
   FrameInterface->AddRef = Surface::AddRef;
   FrameInterface->Release = Surface::Release;
   FrameInterface->GetRefCounter = Surface::GetRefCounter;
   FrameInterface->Map = Surface::Map;
   FrameInterface->Unmap = Surface::Unmap;
   FrameInterface->GetNativeHandle = Surface::GetNativeHandle;
   FrameInterface->GetDeviceHandle = Surface::GetDeviceHandle;
   FrameInterface->Synchronize = Surface::Synchronize;
   FrameInterface->OnComplete = Surface::OnComplete;
   FrameInterface->QueryInterface = Surface::QueryInterface;
}

Surface::~Surface()
{
   delete FrameInterface;

   if (surface_id != VA_INVALID_SURFACE)
      vaDestroySurfaces(session->dpy, &surface_id, 1);
}

bool Surface::create()
{
   uint32_t rt_format = 0;
   uint32_t va_fourcc = 0;
   switch (Info.FourCC) {
   case MFX_FOURCC_NV12:
      rt_format = VA_RT_FORMAT_YUV420;
      va_fourcc = VA_FOURCC_NV12;
      break;
   case MFX_FOURCC_P010:
      rt_format = VA_RT_FORMAT_YUV420_10;
      va_fourcc = VA_FOURCC_P010;
      break;
   case MFX_FOURCC_RGB4:
      rt_format = VA_RT_FORMAT_RGB32;
      va_fourcc = VA_FOURCC_BGRA;
      break;
   case MFX_FOURCC_A2RGB10:
      rt_format = VA_RT_FORMAT_RGB32_10;
      va_fourcc = VA_FOURCC_A2R10G10B10;
      break;
   default:
      break;
   }

   std::vector<VASurfaceAttrib> attribs;

   VASurfaceAttrib attrib;
   attrib.type = VASurfaceAttribPixelFormat;
   attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
   attrib.value.type = VAGenericValueTypeInteger;
   attrib.value.value.i = va_fourcc;
   attribs.push_back(attrib);

   return vaCreateSurfaces(session->dpy, rt_format, session->param.mfx.FrameInfo.Width, session->param.mfx.FrameInfo.Height, &surface_id, 1, attribs.data(), attribs.size()) == VA_STATUS_SUCCESS;
}

mfxStatus Surface::AddRef(mfxFrameSurface1 *surface)
{
   Surface *s = Surface::from_mfx(surface);
   s->ref++;
   return MFX_ERR_NONE;
}

mfxStatus Surface::Release(mfxFrameSurface1 *surface)
{
   Surface *s = Surface::from_mfx(surface);
   if (!--s->ref)
      delete s;
   return MFX_ERR_NONE;
}

mfxStatus Surface::GetRefCounter(mfxFrameSurface1 *surface, mfxU32 *counter)
{
   Surface *s = Surface::from_mfx(surface);
   *counter = s->ref;
   return MFX_ERR_NONE;
}

mfxStatus Surface::Map(mfxFrameSurface1 *surface, mfxU32 flags)
{
   Surface *s = Surface::from_mfx(surface);
   if (vaDeriveImage(s->session->dpy, s->surface_id, &s->image) != VA_STATUS_SUCCESS)
      return MFX_ERR_UNSUPPORTED;
   unsigned vaflags = 0;
   if (flags & MFX_MAP_READ)
      vaflags |= VA_MAPBUFFER_FLAG_READ;
   if (flags & MFX_MAP_WRITE)
      vaflags |= VA_MAPBUFFER_FLAG_WRITE;
   mfxU8 *ptr = nullptr;
   vaMapBuffer2(s->session->dpy, s->image.buf, reinterpret_cast<void **>(&ptr), vaflags);
   if (!ptr) {
      vaDestroyImage(s->session->dpy, s->image.image_id);
      return MFX_ERR_UNKNOWN;
   }
   s->Data.Pitch = s->image.pitches[0];
   s->Data.Y = ptr + s->image.offsets[0];
   s->Data.U = ptr + s->image.offsets[1];
   s->Data.V = ptr + s->image.offsets[2];
   return MFX_ERR_NONE;
}

mfxStatus Surface::Unmap(mfxFrameSurface1 *surface)
{
   Surface *s = Surface::from_mfx(surface);
   if (!s->Data.Y)
      return MFX_ERR_UNSUPPORTED;
   vaUnmapBuffer(s->session->dpy, s->image.buf);
   vaDestroyImage(s->session->dpy, s->image.image_id);
   s->image = {};
   s->Data = {};
   return MFX_ERR_NONE;
}

mfxStatus Surface::GetNativeHandle(mfxFrameSurface1 *surface, mfxHDL *resource, mfxResourceType *resource_type)
{
   Surface *s = Surface::from_mfx(surface);
   *resource = reinterpret_cast<mfxHDL>(s->surface_id);
   if (resource_type)
      *resource_type = MFX_RESOURCE_VA_SURFACE;
   return MFX_ERR_NONE;
}

mfxStatus Surface::GetDeviceHandle(mfxFrameSurface1 *, mfxHDL *, mfxHandleType *)
{
   return MFX_ERR_UNKNOWN;
}

mfxStatus Surface::Synchronize(mfxFrameSurface1 *surface, mfxU32 wait)
{
   Surface *s = Surface::from_mfx(surface);
   VAStatus vas = vaSyncSurface2(s->session->dpy, s->surface_id, wait * 1000);
   if (vas == VA_STATUS_SUCCESS)
      return MFX_ERR_NONE;
   else if (vas == VA_STATUS_ERROR_TIMEDOUT)
      return MFX_WRN_IN_EXECUTION;
   else
      return MFX_ERR_UNKNOWN;
}

void Surface::OnComplete(mfxStatus sts)
{
   std::cout << "OnComplete " << sts << std::endl;
}

mfxStatus Surface::QueryInterface(mfxFrameSurface1 *, mfxGUID, mfxHDL *)
{
   return MFX_ERR_NOT_IMPLEMENTED;
}
