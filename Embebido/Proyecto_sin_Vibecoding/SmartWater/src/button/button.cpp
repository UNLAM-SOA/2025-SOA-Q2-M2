#include "button.h"
#include "Arduino.h"

Button :: Button(){}

Button :: Button(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, INPUT_PULLDOWN);
}

bool Button :: IsPushed() {
  bool lectura_sensor = digitalRead(pinNumber);
  if (ultimo_estado_pulsador == LOW && lectura_sensor == HIGH) {
    delay(50); // Debounce simple
    lectura_sensor = digitalRead(pinNumber); // Verificar nuevamente
    if (lectura_sensor == HIGH) {
      estado_interruptor = !estado_interruptor;
    }
  }
  ultimo_estado_pulsador = lectura_sensor;
  return estado_interruptor;
}