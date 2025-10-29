#include "buzzer.h"
#include "Arduino.h"
#include "utils/utils.h"

Buzzer :: Buzzer(){}

Buzzer :: Buzzer(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, OUTPUT);
    // noTone(pinNumber);
}

void Buzzer :: PlayAlarm(int level){
    int numLevels = sizeof(ALARM_LEVELS) / sizeof(ALARM_LEVELS[0]);
    if (level > 0 && level < numLevels){
        tone(pinNumber, ALARM_LEVELS[level]);
        debug_print("[BUZZER] Play Alarm");
    }
}

void Buzzer :: StopAlarm(){
    noTone(pinNumber);
    debug_print("[BUZZER] Stop Alarm");
}