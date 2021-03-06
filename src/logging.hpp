#pragma once

#include "std_provider/printf.hpp"
#include "std_provider/utility.hpp"

namespace iac {

#ifndef NDEBUG
#    define IAC_DEBUG_BUILD
#endif

#ifdef IAC_DEBUG_BUILD
#    define IAC_LOG_WITH_LINE_NUMBERS
#endif

#ifdef IAC_DISABLE_LOG_WITH_LINE_NUMBERS
#    undef IAC_LOG_WITH_LINE_NUMBERS
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
        COUNT
    } loglevel_t;

    static void set_loglevel(loglevel_t level) {
        s_loglevel = level;
    }

#ifndef IAC_DISABLE_PRINTING
#    define iac_log(level, ...) Logging::log(__FILE__, __LINE__, __FUNCTION__, nullptr, 0, level, __VA_ARGS__);
#    define iac_log_from_node(level, ...) Logging::log(__FILE__, __LINE__, __FUNCTION__, "[node::%d] \t", id(), level, __VA_ARGS__);

    static constexpr void log_head([[maybe_unused]] const char* file, [[maybe_unused]] unsigned line, [[maybe_unused]] const char* function, loglevel_t level, const char* id_str, int id) {
#    ifdef IAC_LOG_WITH_LINE_NUMBERS
        iac_printf(s_log_head_fmt_with_line_info, s_level_colors[(int)level], s_level_names[(int)level], file, line, function);
#    else
        iac_printf(s_log_head_fmt, s_level_colors[(int)level], s_level_names[(int)level]);
#    endif
        if (id_str)
            iac_printf(id_str, id);
    }

    template <typename... Args>
    static constexpr void log(const char* file, unsigned line, const char* function, const char* id_str, int id, loglevel_t level, Args... args) {
        if (s_loglevel >= level) {
            log_head(file, line, function, level, id_str, id);
            iac_printf(args...);
        }
    }

#else
#    define iac_log(level, ...) ;
#endif

   private:
    static loglevel_t s_loglevel;

    static constexpr const uint8_t s_level_colors[(int)loglevels::COUNT] = {31, 33, 34, 36, 35, 32};
    static constexpr const char* s_level_names[(int)loglevels::COUNT] = {"error", "warning", "connect", "network", "debug", "verbose"};

    static constexpr const char* s_log_head_fmt_with_line_info = "\33[%dm[ %-7s %s:%04d in %s]\t>\33[0m";
    static constexpr const char* s_log_head_fmt = "\33[%dm[ %-7s ]\t>\33[0m";
};

}  // namespace iac