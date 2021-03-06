#pragma once

#ifdef IAC_USE_LWSTD
#    include <lw_std/queue.hpp>
namespace iac {
template <typename T>
using queue = lw_std::queue<T>;
}
#else
#    include <queue>
namespace iac {
template <typename T>
using queue = std::queue<T>;
}
#endif