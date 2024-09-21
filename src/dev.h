#pragma once

#include "util.h"

struct enc_dev
{
   enc_dev();
   virtual ~enc_dev();

   bool create(const struct enc_dev_params *params);

   int fd = -1;
   VADisplay dpy = nullptr;
};
