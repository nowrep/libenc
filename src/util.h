#pragma once

#include <va/va.h>
#include <enc/enc.h>
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
