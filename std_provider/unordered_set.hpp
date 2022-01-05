#pragma once

#ifdef IAC_DISABLE_STD
#include "lw_std/unordered_set.hpp"
namespace iac {
template <typename T>
using unordered_set = lw_std::unordered_set<T>;
}
#else
#include <unordered_set>
namespace iac {
template <typename T>
using unordered_set = std::unordered_set<T>;
}
#endif