#pragma once

#include <cstdint>

#ifdef IAC_USE_LWSTD
#    include <lw_std/algorithm.hpp>
#    include <lw_std/utility.hpp>

namespace iac {

template <typename T, typename U>
using pair = lw_std::pair<T, U>;

using lw_std::max_of;
using lw_std::min_of;
using lw_std::move;

}  // namespace iac
#else

#    include <algorithm>
#    include <utility>

namespace iac {

template <typename... Args>
auto min_of(Args&&... args) -> decltype(std::min(std::forward<Args>(args)...)) {
    return std::min(std::forward<Args>(args)...);
}

template <typename... Args>
auto max_of(Args&&... args) -> decltype(std::max(std::forward<Args>(args)...)) {
    return std::max(std::forward<Args>(args)...);
}

template <typename T, typename U>
using pair = std::pair<T, U>;

using std::move;

}  // namespace iac

#endif
