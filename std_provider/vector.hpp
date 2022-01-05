#pragma once

#ifdef IAC_DISABLE_STD
#include "lw_std/vector.hpp"
namespace iac {
template <typename T>
using vector = lw_std::vector<T>;
}
#else
#include <vector>
namespace iac {
template <typename T>
using vector = std::vector<T>;
}
#endif