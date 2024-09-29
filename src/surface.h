#pragma once

#include "util.h"

struct enc_surface
{
   enc_surface();
   virtual ~enc_surface();

   bool create(const struct enc_surface_params *params);
   bool export_dmabuf(struct enc_dmabuf *dmabuf);
   bool copy(struct enc_surface *out);

   enc_dev *dev = nullptr;
   VASurfaceID surface_id = VA_INVALID_ID;
   enc_color_standard color_standard = ENC_COLOR_STANDARD_UNKNOWN;
};
