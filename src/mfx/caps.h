#pragma once

#include <vpl/mfximplcaps.h>

#define VERSION_MAJOR 2
#define VERSION_MINOR 2

mfxHDL *QueryImplsDescription(mfxImplCapsDeliveryFormat format, mfxU32 *num_impls);
mfxStatus ReleaseImplDescription(mfxHDL hdl);
