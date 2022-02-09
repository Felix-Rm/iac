#pragma once

#ifndef IAC_DISABLE_EXCEPTIONS
#    include <exception>
#endif

#ifndef ARDUINO
#    include <cstdlib>
#endif

#include "logging.hpp"

namespace iac {

#ifdef IAC_DISABLE_EXCEPTIONS
#    define IAC_MAKE_EXCEPTION(Name) \
        class Name {};

#else
#    define IAC_MAKE_EXCEPTION(Name)                                   \
        class Name : public std::exception {                           \
           public:                                                     \
            explicit Name(string&& reason) : m_reason(move(reason)){}; \
                                                                       \
            const char* what() const noexcept override {               \
                return m_reason.c_str();                               \
            }                                                          \
                                                                       \
           private:                                                    \
            string m_reason;                                           \
        };

#endif

#ifdef ARDUINO
#    define IAC_HALT() \
        while (true) {}
#else
#    define IAC_HALT() \
        exit(EXIT_FAILURE)
#endif

#define IAC_ASSERT(x)                                                                                                        \
    if (!(x)) {                                                                                                              \
        iac_log(Logging::loglevels::error, "IAC_ASSERT FAILED @ %s:%d in %s(): %s\n", __FILE__, __LINE__, __FUNCTION__, #x); \
        IAC_HALT();                                                                                                          \
    }

#define IAC_ASSERT_NOT_REACHED()                                                                                                        \
    {                                                                                                                                   \
        iac_log(Logging::loglevels::error, "IAC_ASSERT_NOT_REACHED @ %s:%d in%s(): invalid state\n", __FILE__, __LINE__, __FUNCTION__); \
        IAC_HALT();                                                                                                                     \
    }

#define IAC_HANDLE_EXCPETION(type, message) \
    HandleExceptionImpl<type>(__FILE__, __LINE__, __FUNCTION__, message)

#define IAC_HANDLE_FATAL_EXCPETION(type, message) \
    HandleFatalExceptionImpl<type>(__FILE__, __LINE__, __FUNCTION__, message)

template <typename E>
void HandleExceptionImpl(const char* type, int line, const char* function, const char* message) {
    iac_log(Logging::loglevels::error, "run into non-fatal @ %s:%d in %s: %s\n", type, line, function, message);
#ifndef IAC_DISABLE_EXCEPTIONS
    throw E(message);
#endif
}

template <typename E>
void HandleFatalExceptionImpl(const char* type, int line, const char* function, const char* message) {
    iac_log(Logging::loglevels::error, "run into fatal @ %s:%d in %s: %s\n", type, line, function, message);
#ifndef IAC_DISABLE_EXCEPTIONS
    throw E(message);
#endif
    IAC_HALT();
}

}  // namespace iac