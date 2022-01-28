#include "logging.hpp"

namespace iac {

#ifdef IAC_DEBUG_BUILD
Logging::loglevel_t Logging::loglevel = Logging::loglevels::debug;
#else
Logging::loglevel_t Logging::loglevel = Logging::loglevels::connect;
#endif
}  // namespace iac