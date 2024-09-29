#include "dev.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <va/va_drm.h>

enc_dev::enc_dev()
{
}

enc_dev::~enc_dev()
{
   if (proc_context_id != VA_INVALID_ID)
      vaDestroyContext(dpy, proc_context_id);
   if (proc_config_id != VA_INVALID_ID)
      vaDestroyConfig(dpy, proc_config_id);
   if (fd != -1) {
      vaTerminate(dpy);
      close(fd);
   }
}

bool enc_dev::create(const struct enc_dev_params *params)
{
   if (params->va_display) {
      dpy = params->va_display;
   } else {
      fd = open(params->device_path, O_RDWR);
      if (fd == -1) {
         std::cerr << "Failed to open " << params->device_path << ": " << strerror(errno) << std::endl;
         return false;
      }

      dpy = vaGetDisplayDRM(fd);
      if (!dpy) {
         std::cerr << "Failed to get VADisplay\n";
         return false;
      }

      int major_ver, minor_ver;
      VAStatus status = vaInitialize(dpy, &major_ver, &minor_ver);
      if (!va_check(status, "vaInitialize"))
         return false;
   }

   const char *vnd = vaQueryVendorString(dpy);
   if (strstr(vnd, "Mesa Gallium")) {
      if (strstr(vnd, "zink"))
         vendor = VENDOR_ZINK;
      else if (strstr(vnd, "radeonsi") || strstr(vnd, "r600"))
         vendor = VENDOR_AMD;
   } else if (strstr(vnd, "Intel")) {
      vendor = VENDOR_INTEL;
   }

   return true;
}

VAContextID enc_dev::get_proc_context()
{
   if (proc_context_id != VA_INVALID_ID)
      return proc_context_id;

   VAStatus status = vaCreateConfig(dpy, VAProfileNone, VAEntrypointVideoProc, nullptr, 0, &proc_config_id);
   if (!va_check(status, "vaCreateConfig"))
      return VA_INVALID_ID;

   status = vaCreateContext(dpy, proc_config_id, 0, 0, 0, nullptr, 0, &proc_context_id);
   if (!va_check(status, "vaCreateContext"))
      return VA_INVALID_ID;

   return proc_context_id;
}
