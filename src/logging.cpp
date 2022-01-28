#include "logging.hpp"

namespace iac {

constexpr const uint8_t Logging::s_level_colors[(int)loglevels::COUNT];
constexpr const char* Logging::s_level_names[(int)loglevels::COUNT];
constexpr const char* Logging::s_log_head_fmt_with_line_info;
constexpr const char* Logging::s_log_head_fmt;

#ifdef IAC_DEBUG_BUILD
Logging::loglevel_t Logging::s_loglevel = Logging::loglevels::debug;
#else
Logging::loglevel_t Logging::s_loglevel = Logging::loglevels::connect;
#endif
}  // namespace iac