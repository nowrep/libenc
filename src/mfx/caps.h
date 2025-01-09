#pragma once

#include <vpl/mfximplcaps.h>

mfxHDL *QueryImplsDescription(mfxImplCapsDeliveryFormat format, mfxU32 *num_impls);
mfxStatus ReleaseImplDescription(mfxHDL hdl);
