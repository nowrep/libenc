#pragma once

#include <va/va.h>
#include <enc/enc.h>
#include <math.h>
#include <utility>
#include <iostream>

static inline bool va_check(VAStatus status, const char *func)
{
   if (status == VA_STATUS_SUCCESS)
      return true;
   std::cerr << func << " failed: " << vaErrorStr(status) << std::endl;
   return false;
}

template <typename T>
static inline T align(T x, uint32_t a)
{
   return (x + a - 1) & ~(a - 1);
}

static inline std::pair<uint32_t, uint32_t> get_framerate(float fps)
{
   uint32_t num = fps;
   uint32_t den = 1;
   if (fps - round(fps) > 0.0001) {
      num = fps * 1000;
      den = 1000;
   }
   return {num, den};
}
