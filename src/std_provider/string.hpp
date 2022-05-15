#pragma once

#include <cstring>
#ifdef IAC_USE_LWSTD
#    include <lw_std/string.hpp>
namespace iac {
using string = lw_std::string;
using lw_std::to_string;
}  // namespace iac
#else
#    include <cstring>
#    include <string>
namespace iac {
using string = std::string;
using std::to_string;
}  // namespace iac
#endif