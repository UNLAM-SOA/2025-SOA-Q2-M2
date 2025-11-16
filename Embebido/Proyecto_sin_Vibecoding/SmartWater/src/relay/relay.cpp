#include "relay.h"
#include <Arduino.h>

Relay :: Relay(){}

Relay :: Relay(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, OUTPUT);
    this->isActive = false;
}

void Relay :: activate(){
    digitalWrite(pinNumber, HIGH);
    this->isActive = true;
}

void Relay :: deactivate(){
    digitalWrite(pinNumber, LOW);
    this->isActive = false;
}

bool Relay :: IsActive(){
    return this->isActive;
}