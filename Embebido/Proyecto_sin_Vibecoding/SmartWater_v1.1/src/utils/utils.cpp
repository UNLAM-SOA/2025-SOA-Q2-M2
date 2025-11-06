#include "utils.h"
#include "HardwareSerial.h"

void debug_print(const char* msg) {
  Serial.println(msg);
}

void debug_printf(const char *format, ...) {
  char buffer[256]; 
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args); 
  va_end(args);
  Serial.print(buffer);
}
