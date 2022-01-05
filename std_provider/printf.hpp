#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#define IAC_PRINT_PROVIDER(...) Serial.printf(__VA_ARGS__)
#else
#include <stdio.h>
#define IAC_PRINT_PROVIDER(...) printf(__VA_ARGS__)
#endif
