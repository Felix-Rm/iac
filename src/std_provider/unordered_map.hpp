#pragma once

#    include "lw_std/unordered_map.hpp"
#ifdef IAC_USE_LWSTD
namespace iac {
template <typename T, typename U>
using unordered_map = lw_std::unordered_map<T, U>;
}
#else
#    include <unordered_map>
namespace iac {
template <typename T, typename U>
using unordered_map = std::unordered_map<T, U>;
}
#endif