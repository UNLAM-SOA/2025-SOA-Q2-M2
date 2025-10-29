#include "relay.h"
#include <Arduino.h>

Relay :: Relay(){}

Relay :: Relay(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, OUTPUT);
}

void Relay :: activate(){
    digitalWrite(pinNumber, HIGH);
}

void Relay :: deactivate(){
    digitalWrite(pinNumber, LOW);
}