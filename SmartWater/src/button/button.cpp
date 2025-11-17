#include "button.h"
#include "Arduino.h"

Button :: Button(){}

Button :: Button(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, INPUT_PULLDOWN);
}

bool Button :: IsPushed() {
  return digitalRead(pinNumber) == HIGH;
}