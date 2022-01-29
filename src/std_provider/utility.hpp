#pragma once

#include <cstdlib>

#include "std_provider/string.hpp"

#ifdef IAC_DISABLE_STD
#    include "lw_std/utility.hpp"

namespace iac {

template <typename T, typename U>
using pair = lw_std::pair<T, U>;

using lw_std::max;
using lw_std::min;
using lw_std::move;

}  // namespace iac
#else

#    include <algorithm>
#    include <utility>

namespace iac {

template <typename... Args>
auto min(Args&&... args) -> decltype(std::min(std::forward<Args>(args)...)) {
    return std::min(std::forward<Args>(args)...);
}

template <typename... Args>
auto max(Args&&... args) -> decltype(std::max(std::forward<Args>(args)...)) {
    return std::max(std::forward<Args>(args)...);
}

template <typename T, typename U>
using pair = std::pair<T, U>;

using std::move;

}  // namespace iac

#endif
