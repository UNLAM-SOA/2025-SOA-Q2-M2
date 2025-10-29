#include "flowmeter.h"
#include "Arduino.h"

// Variables para conteo de pulsos (volatile porque se usan en ISR)
volatile unsigned long pulsos_totales = 0;
volatile unsigned long pulsos_muestra = 0;

unsigned long tiempo_ultima_muestra = 0;
long TIEMPO_MUESTRA_CAUDAL_MS = 1000;
float FACTOR_K = 3.75;

void IRAM_ATTR contarPulsos() {
  pulsos_totales++;
  pulsos_muestra++;
}

FlowMeter :: FlowMeter(){}

FlowMeter :: FlowMeter(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, INPUT);
    attachInterrupt(digitalPinToInterrupt(pinNumber), contarPulsos, RISING);
}

float FlowMeter :: GetConsumptionSinceLastMeasurement(){
  unsigned long tiempo_actual = millis();
  unsigned long tiempo_transcurrido = tiempo_actual - tiempo_ultima_muestra;
  
  // Actualizar cada segundo (o según TIEMPO_MUESTRA_CAUDAL_MS)
  if (tiempo_transcurrido >= TIEMPO_MUESTRA_CAUDAL_MS) {
    // Calcular frecuencia de pulsos en Hz
    float frecuencia = (pulsos_muestra * 1000.0) / tiempo_transcurrido;
    
    // Convertir frecuencia a caudal en L/min
    float caudal_actual = frecuencia / FACTOR_K;
    
    // Calcular volumen incremental: Q(L/min) * tiempo(min)
    float tiempo_min = tiempo_transcurrido / 60000.0;
    float volumen_incremental = caudal_actual * tiempo_min;
    
    // Resetear contadores para la próxima muestra
    pulsos_muestra = 0;
    tiempo_ultima_muestra = tiempo_actual;

    return volumen_incremental;
  }
  return 0;
}