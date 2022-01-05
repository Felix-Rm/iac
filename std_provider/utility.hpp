#pragma once

#include <cstdlib>

#include "../std_provider/string.hpp"

#ifdef IAC_DISABLE_STD
#include "lw_std/utility.hpp"

namespace iac {

template <typename T, typename U>
using pair = lw_std::pair<T, U>;

using lw_std::max;
using lw_std::min;
using lw_std::move;

}  // namespace iac
#else

#include <algorithm>
#include <exception>
#include <utility>

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

namespace iac {

#ifdef ARDUINO
#define IAC_TERMINATE() \
    while (true)        \
        ;
#else
#define IAC_TERMINATE() exit(EXIT_FAILURE);
#endif

#define IAC_STRINGIFY(x) #x
#define IAC_TOSTRING(x) IAC_STRINGIFY(x)
#define IAC_AT __FILE__ ":" IAC_TOSTRING(__LINE__)
#define IAC_PRINT_EXCEPTION(type, message) iac_log(error, "run into non-fatal " #type \
                                                          " @ " IAC_AT " message: " message "\n");

#define IAC_ASSERT(x)                                                             \
    if (!(x)) {                                                                   \
        iac_log(error, "IAC_ASSERT FAILED @ " IAC_AT " - " IAC_TOSTRING(x) "\n"); \
        IAC_TERMINATE();                                                          \
    }

#define IAC_ASSERT_NOT_REACHED()                                              \
    iac_log(error, "IAC_ASSERT_NOT_REACHED @ " IAC_AT " - invalid state \n"); \
    IAC_TERMINATE();

#ifndef IAC_DISABLE_EXCEPTIONS

#define IAC_HANDLE_EXCEPTION(type, message) \
    IAC_PRINT_EXCEPTION(type, message);     \
    throw type(message);

#define IAC_HANDLE_FATAL_EXCEPTION(type, message) IAC_HANDLE_EXCEPTION(type, message)

#define IAC_CREATE_MESSAGE_EXCEPTION(name)         \
    class name : public Exception {                \
       public:                                     \
        name(string reason) : Exception(reason){}; \
    }

class Exception : public std::exception {
   public:
    Exception(string reason) : m_reason(reason){};

    const char* what() const noexcept override {
        return m_reason.c_str();
    }

   private:
    string m_reason;
};

#else

#define IAC_HANDLE_EXCEPTION(type, message)

#define IAC_HANDLE_FATAL_EXCEPTION(type, message) \
    IAC_HANDLE_EXCEPTION(type, message);          \
    while (true) {                                \
    }

#define IAC_CREATE_MESSAGE_EXCEPTION(name)
#endif
}  // namespace iac