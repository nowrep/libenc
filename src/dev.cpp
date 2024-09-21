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
   if (dpy)
      vaTerminate(dpy);
   if (fd != -1)
      close(fd);
}

bool enc_dev::create(const struct enc_dev_params *params)
{
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
   return va_check(status, "vaInitialize");
}
