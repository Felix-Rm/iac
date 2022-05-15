#pragma once

#    include "lw_std/unordered_set.hpp"
#ifdef IAC_USE_LWSTD
namespace iac {
template <typename T>
using unordered_set = lw_std::unordered_set<T>;
}
#else
#    include <unordered_set>
namespace iac {
template <typename T>
using unordered_set = std::unordered_set<T>;
}
#endif