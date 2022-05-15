#pragma once

#    include "lw_std/limits.hpp"
#ifdef IAC_USE_LWSTD
namespace iac {
template <typename T>
using numeric_limits = lw_std::numeric_limits<T>;
}
#else
#    include <limits>
namespace iac {
template <typename T>
using numeric_limits = std::numeric_limits<T>;
}
#endif