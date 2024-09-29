#pragma once

#include "util.h"

struct enc_dev
{
   enum vendor {
      VENDOR_UNKNOWN = 0,
      VENDOR_AMD,
      VENDOR_INTEL,
      VENDOR_ZINK,
   };

   enc_dev();
   virtual ~enc_dev();

   bool create(const struct enc_dev_params *params);
   VAContextID get_proc_context();

   int fd = -1;
   VADisplay dpy = nullptr;
   vendor vendor = VENDOR_UNKNOWN;

   VAConfigID proc_config_id = VA_INVALID_ID;
   VAContextID proc_context_id = VA_INVALID_ID;
};
