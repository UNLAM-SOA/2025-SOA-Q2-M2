#include "buzzer.h"
#include "Arduino.h"
#include "utils/utils.h"

const int NUMBER_OF_ALARM_LEVELS = 5;
const int ALARM_LEVELS[NUMBER_OF_ALARM_LEVELS] = {
  0,        // Level 0: No tone
  5000,     // Level 1: Low frequency
  7000,     // Level 2: Medium frequency
  9000,     // Level 3: High frequency
  12000     // Level 4: Max frequency
};

Buzzer :: Buzzer(){}

Buzzer :: Buzzer(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, OUTPUT);
}

void Buzzer :: PlayAlarm(int level){
    if (level > 0 && level < NUMBER_OF_ALARM_LEVELS){
        tone(pinNumber, ALARM_LEVELS[level]);
        debug_print("[BUZZER] Play Alarm");
    }
}

void Buzzer :: StopAlarm(){
    noTone(pinNumber);
    debug_print("[BUZZER] Stop Alarm");
}