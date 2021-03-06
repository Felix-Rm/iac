#pragma once

#ifdef IAC_USE_LWSTD
#    include <lw_std/limits.hpp>
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