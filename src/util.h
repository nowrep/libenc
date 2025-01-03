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

template <typename T>
static inline T align_npot(T x, uint32_t a)
{
   return x % a ? x + (a - (x % a)) : x;
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

static inline uint32_t logbase2(uint32_t v)
{
   return log(v) / log(2.0);
}

static inline uint32_t logbase2_ceil(uint32_t v)
{
   return ceil(log(v) / log(2.0));
}

template <typename T>
static inline T div_round_up(T a, T b)
{
   return (a + b - 1) / b;
}
