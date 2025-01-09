#include "caps.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

#include <va/va.h>
#include <va/va_drm.h>
#include <xf86drm.h>

static const mfxChar *g_functions[] = {
   "MFXQueryIMPL",
   "MFXQueryVersion",
   "MFXVideoCORE_SetHandle",
   "MFXVideoCORE_SyncOperation",
   "MFXVideoENCODE_Query",
   "MFXVideoENCODE_QueryIOSurf",
   "MFXVideoENCODE_Init",
   "MFXVideoENCODE_Reset",
   "MFXVideoENCODE_Close",
   "MFXVideoENCODE_GetVideoParam",
   "MFXVideoENCODE_EncodeFrameAsync",
   "MFXQueryImplsDescription",
   "MFXReleaseImplDescription",
   "MFXInitialize",
};

struct caps
{
   virtual ~caps() {};

   static void destroy(mfxHDL hdl)
   {
      delete reinterpret_cast<caps *>(reinterpret_cast<mfxU8 *>(hdl) - sizeof(caps));
   }
};

template <typename T>
struct array
{
   void push_back(T *&dst, const std::vector<T> &val)
   {
      data.push_back(val);
      dst = data.back().data();
   }

   std::vector<std::vector<T>> data;
};

struct impl_description : public caps, public mfxImplDescription
{
   mfxImplDescription *to_mfx()
   {
      return static_cast<mfxImplDescription *>(this);
   }

   std::vector<mfxImplDescription*> out;
   mfxPoolAllocationPolicy pool_policy;
   array<mfxEncoderDescription::encoder> encoders;
   array<mfxEncoderDescription::encoder::encprofile> profiles;
   array<mfxEncoderDescription::encoder::encprofile::encmemdesc> mems;
   array<mfxU32> colorfmts;
};

struct impl_functions : public caps, public mfxImplementedFunctions
{
   mfxImplementedFunctions *to_mfx()
   {
      return static_cast<mfxImplementedFunctions *>(this);
   }

   std::vector<mfxImplementedFunctions*> out;
};

struct impl_devices : public caps, public mfxExtendedDeviceId
{
   mfxExtendedDeviceId *to_mfx()
   {
      return static_cast<mfxExtendedDeviceId *>(this);
   }

   std::vector<mfxExtendedDeviceId*> out;
};

enum codec_idx
{
   CODEC_AVC = 0,
   CODEC_HEVC,
   CODEC_AV1,
   CODEC_MAX
};

static mfxU32 codec_idx_to_mfx[CODEC_MAX] = {
   MFX_CODEC_AVC,
   MFX_CODEC_HEVC,
   MFX_CODEC_AV1,
};

static std::unique_ptr<impl_description> query_impl_va(VADisplay dpy, mfxU32 id)
{
   auto impl = std::make_unique<impl_description>();
   impl->Version.Version = MFX_STRUCT_VERSION(1, 0);
   impl->Impl = MFX_IMPL_TYPE_HARDWARE;
   impl->AccelerationMode = MFX_ACCEL_MODE_VIA_VAAPI;
   impl->ApiVersion = { { 0, 2 } };
   snprintf(impl->ImplName, sizeof(impl->ImplName), "libenc");
   snprintf(impl->License, sizeof(impl->License), "MIT License");

   VADisplayAttribute disp_attrib = {};
   disp_attrib.type = VADisplayPCIID;
   vaGetDisplayAttributes(dpy, &disp_attrib, 1);

   impl->VendorID = disp_attrib.value >> 16;
   impl->VendorImplID = id;
   impl->Dev.Version.Version = MFX_STRUCT_VERSION(1, 1);
   impl->Dev.MediaAdapterType = MFX_MEDIA_UNKNOWN;
   snprintf(impl->Dev.DeviceID, sizeof(impl->Dev.DeviceID), "%x/%d", disp_attrib.value & 0xFFFF, id);

   impl->AccelerationModeDescription.Version.Version = MFX_STRUCT_VERSION(1, 0);
   impl->AccelerationModeDescription.NumAccelerationModes = 1;
   impl->AccelerationModeDescription.Mode = &impl->AccelerationMode;

   impl->PoolPolicies.Version.Version = MFX_STRUCT_VERSION(1, 0);
   impl->PoolPolicies.NumPoolPolicies = 1;
   impl->pool_policy = MFX_ALLOCATION_OPTIMAL;
   impl->PoolPolicies.Policy = &impl->pool_policy;

   impl->Enc.Version.Version = MFX_STRUCT_VERSION(1, 0);

   std::vector<VAProfile> profiles(vaMaxNumProfiles(dpy));
   std::vector<VAEntrypoint> entrypoints(vaMaxNumEntrypoints(dpy));
   std::vector<VAProfile> codec_profiles[CODEC_MAX];
   std::vector<mfxU32> mfx_profiles[VAProfileAV1Profile0 + 1];
   int num_profiles = 0;
   vaQueryConfigProfiles(dpy, profiles.data(), &num_profiles);
   for (int i = 0; i < num_profiles; i++) {
      int num_entrypoints = 0;
      vaQueryConfigEntrypoints(dpy, profiles[i], entrypoints.data(), &num_entrypoints);
      bool have_enc = false;
      for (int j = 0; j < num_entrypoints; j++) {
         if (entrypoints[j] == VAEntrypointEncSlice)
            have_enc = true;
      }
      if (!have_enc)
         continue;
      switch (profiles[i]) {
      case VAProfileH264ConstrainedBaseline:
      case VAProfileH264Main:
      case VAProfileH264High:
         codec_profiles[CODEC_AVC].push_back(profiles[i]);
         break;
      case VAProfileHEVCMain:
      case VAProfileHEVCMain10:
         codec_profiles[CODEC_HEVC].push_back(profiles[i]);
         break;
      case VAProfileAV1Profile0:
         codec_profiles[CODEC_AV1].push_back(profiles[i]);
         break;
      default:
         break;
      }
   }

   std::vector<mfxEncoderDescription::encoder> encoders;

   for (int i = 0; i < CODEC_MAX; i++) {
      if (codec_profiles[i].empty())
         continue;
      mfxEncoderDescription::encoder enc = {};
      enc.CodecID = codec_idx_to_mfx[i];
      enc.BiDirectionalPrediction = 0;
      std::vector<mfxEncoderDescription::encoder::encprofile> profiles;
      for (auto profile : codec_profiles[i]) {
         VAConfigID config = VA_INVALID_ID;
         if (vaCreateConfig(dpy, profile, VAEntrypointEncSlice, nullptr, 0, &config) != VA_STATUS_SUCCESS)
            continue;
         mfxEncoderDescription::encoder::encprofile prof = {};
         switch (profile) {
         case VAProfileH264ConstrainedBaseline:
            prof.Profile = MFX_PROFILE_AVC_CONSTRAINED_BASELINE;
            enc.MaxcodecLevel = MFX_LEVEL_AVC_52;
            break;
         case VAProfileH264Main:
            prof.Profile = MFX_PROFILE_AVC_MAIN;
            enc.MaxcodecLevel = MFX_LEVEL_AVC_52;
            break;
         case VAProfileH264High:
            prof.Profile = MFX_PROFILE_AVC_HIGH;
            enc.MaxcodecLevel = MFX_LEVEL_AVC_52;
            break;
         case VAProfileHEVCMain:
            prof.Profile = MFX_PROFILE_HEVC_MAIN;
            enc.MaxcodecLevel = MFX_LEVEL_HEVC_85;
            break;
         case VAProfileHEVCMain10:
            prof.Profile = MFX_PROFILE_HEVC_MAIN10;
            enc.MaxcodecLevel = MFX_LEVEL_HEVC_85;
            break;
         case VAProfileAV1Profile0:
            prof.Profile = MFX_PROFILE_AV1_MAIN;
            enc.MaxcodecLevel = MFX_LEVEL_AV1_6;
            break;
         default:
            break;
         }
         unsigned num_attributes = 0;
         vaQuerySurfaceAttributes(dpy, config, nullptr, &num_attributes);
         std::vector<VASurfaceAttrib> attribs(num_attributes);
         vaQuerySurfaceAttributes(dpy, config, attribs.data(), &num_attributes);
         std::vector<mfxEncoderDescription::encoder::encprofile::encmemdesc> mems;
         mfxEncoderDescription::encoder::encprofile::encmemdesc mem = {};
         mem.MemHandleType = MFX_RESOURCE_SYSTEM_SURFACE;
         mem.Width.Step = 16;
         mem.Height.Step = 16;
         std::vector<mfxU32> colorfmts;
         for (auto attrib : attribs) {
            switch (attrib.type) {
            case VASurfaceAttribPixelFormat:
               mem.NumColorFormats++;
               colorfmts.push_back(attrib.value.value.i);
               break;
            case VASurfaceAttribMinWidth:
               mem.Width.Min = attrib.value.value.i;
               break;
            case VASurfaceAttribMaxWidth:
               mem.Width.Max = attrib.value.value.i;
               break;
            case VASurfaceAttribMinHeight:
               mem.Height.Min = attrib.value.value.i;
               break;
            case VASurfaceAttribMaxHeight:
               mem.Height.Max = attrib.value.value.i;
               break;
            case VASurfaceAttribAlignmentSize:
               mem.Width.Step = 1 << (attrib.value.value.i & 0xF);
               mem.Height.Step = 1 << ((attrib.value.value.i & 0xF0) >> 4);
               break;
            default:
               break;
            }
         }
         impl->colorfmts.push_back(mem.ColorFormats, colorfmts);

         prof.NumMemTypes++;
         mems.push_back(mem);

         mem.MemHandleType = MFX_RESOURCE_VA_SURFACE;
         prof.NumMemTypes++;
         mems.push_back(mem);

         impl->mems.push_back(prof.MemDesc, mems);

         enc.NumProfiles++;
         profiles.push_back(prof);

         vaDestroyConfig(dpy, config);
      }
      impl->profiles.push_back(enc.Profiles, profiles);
      impl->Enc.NumCodecs++;
      encoders.push_back(enc);
   }

   impl->encoders.push_back(impl->Enc.Codecs, encoders);

   return impl;
}

static void log_dummy(void *, const char *)
{
}

static void query_devices(std::function<void(int, int)> fun)
{
   for (int i = 0; i < 64; i++) {
      auto path = "/dev/dri/renderD" + std::to_string(128 + i);
      if (!std::filesystem::exists(path))
         continue;

      int fd = open(path.c_str(), O_RDWR);
      if (fd < 0)
         continue;

      drmVersionPtr ver = drmGetVersion(fd);
      if (!ver) {
         close(fd);
         continue;
      }
      bool supported_driver = strcmp("amdgpu", ver->name) == 0;
      drmFreeVersion(ver);

      if (supported_driver)
         fun(i, fd);

      close(fd);
   }
}

static mfxImplDescription **query_impl(mfxU32 *num_impls)
{
   impl_description *impl = nullptr;

   query_devices([&](int id, int fd) {

      VADisplay dpy = vaGetDisplayDRM(fd);
      if (!dpy)
         return;

      vaSetInfoCallback(dpy, log_dummy, nullptr);
      vaSetErrorCallback(dpy, log_dummy, nullptr);

      int minor, major;
      vaInitialize(dpy, &minor, &major);

      if (auto im = query_impl_va(dpy, id)) {
         if (!impl)
            impl = im.get();
         impl->out.push_back(im.release()->to_mfx());
         (*num_impls)++;
      }

      vaTerminate(dpy);
   });

   return impl ? impl->out.data() : nullptr;
}

static mfxImplementedFunctions **query_functions(mfxU32 *num_impls)
{
   impl_functions *impl = nullptr;

   query_devices([&](int, int) {
      auto im = std::make_unique<impl_functions>();
      im->FunctionsName = const_cast<mfxChar **>(g_functions);
      im->NumFunctions = sizeof(g_functions) / sizeof(g_functions[0]);

      if (!impl)
         impl = im.get();
      impl->out.push_back(im.release()->to_mfx());
      (*num_impls)++;
   });

   return impl ? impl->out.data() : nullptr;
}

static mfxExtendedDeviceId **query_device_id(mfxU32 *num_impls)
{
   impl_devices *impl = nullptr;

   query_devices([&](int id, int fd) {
      drmDevicePtr dev = nullptr;
      drmGetDevice(fd, &dev);
      if (!dev || dev->bustype != DRM_BUS_PCI)
         abort();

      auto im = std::make_unique<impl_devices>();
      im->Version.Version = MFX_STRUCT_VERSION(1, 0);
      im->VendorID = dev->deviceinfo.pci->vendor_id;
      im->DeviceID = dev->deviceinfo.pci->device_id;
      im->PCIDomain = dev->businfo.pci->domain;
      im->PCIBus = dev->businfo.pci->bus;
      im->PCIDevice = dev->businfo.pci->dev;
      im->DRMRenderNodeNum = id + 128;
      im->DRMPrimaryNodeNum = id;
      im->RevisionID = dev->deviceinfo.pci->revision_id;
      snprintf(im->DeviceName, sizeof(im->DeviceName), "libenc");

      drmFreeDevice(&dev);

      if (!impl)
         impl = im.get();
      impl->out.push_back(im.release()->to_mfx());
      (*num_impls)++;
   });

   return impl ? impl->out.data() : nullptr;
}

mfxHDL *QueryImplsDescription(mfxImplCapsDeliveryFormat format, mfxU32 *num_impls)
{
   if (!num_impls)
      return nullptr;

   *num_impls = 0;

   switch (format) {
   case MFX_IMPLCAPS_IMPLDESCSTRUCTURE:
      return reinterpret_cast<mfxHDL *>(query_impl(num_impls));
   case MFX_IMPLCAPS_IMPLEMENTEDFUNCTIONS:
      return reinterpret_cast<mfxHDL *>(query_functions(num_impls));
   case MFX_IMPLCAPS_DEVICE_ID_EXTENDED:
      return reinterpret_cast<mfxHDL *>(query_device_id(num_impls));
   default:
      return nullptr;
   }
}

mfxStatus ReleaseImplDescription(mfxHDL hdl)
{
   caps::destroy(hdl);
   return MFX_ERR_NONE;
}
