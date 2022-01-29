#pragma once

#ifndef IAC_DISABLE_PRINTING

#    ifdef ARDUINO
#        include <Arduino.h>
#    else
#        include <cstdio>
#    endif

namespace iac {

template <typename... Args>
inline constexpr int iac_printf(Args... args) {
#    ifdef ARDUINO
    return Serial.printf(args...);
#    else
    return printf(args...);
#    endif
}
}  // namespace iac

#endif
