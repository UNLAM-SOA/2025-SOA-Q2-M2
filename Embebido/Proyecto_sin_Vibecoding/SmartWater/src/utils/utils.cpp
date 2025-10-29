#include "utils.h"
#include "HardwareSerial.h"

void debug_print(const char* msg) {
  Serial.println(msg);
}