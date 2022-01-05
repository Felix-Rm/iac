#pragma once

#include "std_provider/printf.hpp"

namespace iac {

#ifndef NDEBUG
#define IAC_DEBUG_BUILD
#endif

#ifdef IAC_DEBUG_BUILD
#define IAC_LOG_WITH_LINE_NUMBERS
#endif

#ifdef IAC_DISABLE_LOG_WITH_LINE_NUMBERS
#undef IAC_LOG_WITH_LINE_NUMBERS
#endif

#ifdef IAC_PRINT_PROVIDER
#define iac_log(level, ...) IAC_LOG_PROVIDER(__FILE__, __LINE__, level, __VA_ARGS__);

#ifdef IAC_LOG_WITH_LINE_NUMBERS
#define IAC_LOG_HEAD_PROVIDER(file, line, level) \
    IAC_PRINT_PROVIDER("\33[%dm[ %-7s %s:%04d ]\t\33[0m ", ::iac::Logging::fmt_strings[(int)::iac::Logging::loglevels::level], #level, file, line);

#else
#define IAC_LOG_HEAD_PROVIDER(file, line, level) \
    IAC_PRINT_PROVIDER("\33[%dm[ %s ]\t\33[0m ", ::iac::Logging::fmt_strings[(int)::iac::Logging::loglevels::level], #level);

#endif

#define IAC_LOG_PROVIDER(file, line, level, ...)                        \
    if (::iac::Logging::loglevel >= ::iac::Logging::loglevels::level) { \
        IAC_LOG_HEAD_PROVIDER(file, line, level);                       \
        IAC_PRINT_PROVIDER(__VA_ARGS__);                                \
    }

#else
#define iac_log(level, ...) ;
#endif

class Logging {
   public:
    typedef enum class loglevels {
        error,
        warning,
        connect,
        network,
        debug,
        verbose,
        _COUNT
    } loglevel_t;

    static loglevel_t loglevel;

    constexpr static const char fmt_strings[(int)loglevels::_COUNT] = {31, 33, 34, 36, 35, 32};
};

}  // namespace iac