#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <stdarg.h>

void debug_print(const char* msg);
void debug_printf(const char *format, ...);

#endif