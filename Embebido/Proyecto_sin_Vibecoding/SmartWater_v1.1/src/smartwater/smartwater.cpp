#include "utils/utils.h"
#include "smartwater.h"
#include <Arduino.h>

#define CANT_EVENTOS_POSIBLES 6
func_evento_t EVENTOS_POSIBLES[CANT_EVENTOS_POSIBLES] = {
  detectar_button_on,
  detectar_consumo_umbral1,
  detectar_consumo_umbral2,
  detectar_consumo_umbral3,
  detectar_consumo_limite,
  detectar_timeout_alarma
};

bool detectar_button_on(SmartWater* s, stEvento* e) {
  return s->detectar_button_on(e);
};

bool detectar_consumo_umbral1(SmartWater* s, stEvento* e) {
  return s->detectar_consumo_umbral1(e);
};

bool detectar_consumo_umbral2(SmartWater* s, stEvento* e) {
  return s->detectar_consumo_umbral2(e);
};

bool detectar_consumo_umbral3(SmartWater* s, stEvento* e) {
  return s->detectar_consumo_umbral3(e);
};

bool detectar_consumo_limite(SmartWater* s, stEvento* e) {
  return s->detectar_consumo_limite(e);
};

bool detectar_timeout_alarma(SmartWater* s, stEvento* e) {
  return s->detectar_timeout_alarma(e);
}

SmartWater :: SmartWater(){}

SmartWater :: SmartWater(
    const int PIN_DIGITAL_PULSADOR,
    const int PIN_DIGITAL_CAUDALIMETRO,
    const int PIN_PWM_BUZZER,
    const int PIN_DIGITAL_RELAY_ELECTROVALVULA,
    const float UMBRAL_CONSUMO_1,
    const float UMBRAL_CONSUMO_2,
    const float UMBRAL_CONSUMO_3,
    const float UMBRAL_LIMITE,
    const char* MQTT_TOPIC_NAME,
    const char* MQTT_SERVER,
    const int MQTT_PORT,
    const long INTERVALO_DETECCION_MS,
    const long INTERVALO_DURACION_ALARMA
){
    button = Button(PIN_DIGITAL_PULSADOR);

    buzzer = Buzzer(PIN_PWM_BUZZER);

    flowmeter = FlowMeter(PIN_DIGITAL_CAUDALIMETRO);

    valve = Relay(PIN_DIGITAL_RELAY_ELECTROVALVULA);

    display = LCDDisplay(500);

    this->INTERVALO_DETECCION_MS = INTERVALO_DETECCION_MS;
    this->UMBRAL_CONSUMO_1 = UMBRAL_CONSUMO_1;
    this->UMBRAL_CONSUMO_2 = UMBRAL_CONSUMO_2;
    this->UMBRAL_CONSUMO_3 = UMBRAL_CONSUMO_3;
    this->UMBRAL_LIMITE = UMBRAL_LIMITE;
    this->INTERVALO_DURACION_ALARMA = INTERVALO_DURACION_ALARMA;

    debug_print("==============================================");
    debug_print("[SISTEMA] SmartWater inicializado correctamente");
    debug_print("==============================================");
    display.Write("SmartWater", "Grupo M2");
}

// === State machine ===
void SmartWater :: FSM(stEvento event){
  switch (ESTADO_ACTUAL) {
    case ESTADO_IDLE:
      switch (event.tipo) {
        case TIPO_EVENTO_BUTTON_ON:
          debug_print("[FSM] IDLE -> APERTURA_ELECTROVALVULA");
          valve.activate();
          ESTADO_ACTUAL = ESTADO_APERTURA_ELECTROVALVULA;
          break;
      }
      break;
      
    case ESTADO_APERTURA_ELECTROVALVULA:
      switch (event.tipo) {
        case TIPO_EVENTO_BUTTON_ON:
          debug_print("[FSM] APERTURA_ELECTROVALVULA -> IDLE");
          valve.deactivate();
          ESTADO_ACTUAL = ESTADO_IDLE;
          break;

        case TIPO_EVENTO_TIMEOUT_VALVULA: // CHECKEAR
          debug_print("[FSM] APERTURA_ELECTROVALVULA -> NORMAL");
          ESTADO_ACTUAL = ESTADO_IDLE;
          break;

        case TIPO_EVENTO_CONSUMO_UMBRAL1:
          debug_print("[FSM] NORMAL -> ALARMA1");
          buzzer.PlayAlarm(1);
          ESTADO_ACTUAL = ESTADO_ALARMA1;
          TIEMPO_INICIO_ALARMA = millis();
          break;

        case TIPO_EVENTO_CONSUMO_UMBRAL2:
          debug_print("[FSM] NORMAL -> ALARMA2");
          buzzer.PlayAlarm(2);
          ESTADO_ACTUAL = ESTADO_ALARMA2;
          TIEMPO_INICIO_ALARMA = millis();
          break;

        case TIPO_EVENTO_CONSUMO_UMBRAL3:
          debug_print("[FSM] NORMAL -> ALARMA3");
          buzzer.PlayAlarm(3);
          ESTADO_ACTUAL = ESTADO_ALARMA3;
          TIEMPO_INICIO_ALARMA = millis();
          break;

        case TIPO_EVENTO_CONSUMO_LIMITE:
          debug_print("[FSM] NORMAL -> ALARMA4");
          buzzer.PlayAlarm(4);
          ESTADO_ACTUAL = ESTADO_LIMITE;
          TIEMPO_INICIO_ALARMA = millis();
          break;
      }
      break;
            
    case ESTADO_ALARMA1:
      switch (event.tipo) {
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA1 -> NORMAL");
          buzzer.StopAlarm();
          ESTADO_ACTUAL = ESTADO_APERTURA_ELECTROVALVULA;
          break;

        case TIPO_EVENTO_CONSUMO_UMBRAL2:
          debug_print("[FSM] ALARMA1 -> ALARMA2");
          buzzer.PlayAlarm(2);
          ESTADO_ACTUAL = ESTADO_ALARMA2;
          break;
      }
      break;
      
    case ESTADO_ALARMA2:
      switch (event.tipo) {
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA2 -> NORMAL");
          buzzer.StopAlarm();
          ESTADO_ACTUAL = ESTADO_APERTURA_ELECTROVALVULA;
          break;

        case TIPO_EVENTO_CONSUMO_UMBRAL3:
          debug_print("[FSM] ALARMA2 -> ALARMA3");
          buzzer.PlayAlarm(3);
          ESTADO_ACTUAL = ESTADO_ALARMA3;
          break;
      }
      break;
      
    case ESTADO_ALARMA3:
      switch (event.tipo) {
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA3 -> NORMAL");
          buzzer.StopAlarm();
          ESTADO_ACTUAL = ESTADO_APERTURA_ELECTROVALVULA;
          break;

        case TIPO_EVENTO_CONSUMO_LIMITE:
          debug_print("[FSM] ALARMA3 -> ALARMA4");
          buzzer.PlayAlarm(4);
          ESTADO_ACTUAL = ESTADO_LIMITE;
          break;
      }
      break;
      
    case ESTADO_LIMITE:
      if (event.tipo == TIPO_EVENTO_TIMEOUT_ALARMA) {
        debug_print("[FSM] ALARMA4 -> IDLE (LÍMITE ALCANZADO)");
        buzzer.StopAlarm();
        valve.deactivate();
        ESTADO_ACTUAL = ESTADO_FINALIZADO;
      }
      break;

    case ESTADO_FINALIZADO:
      if (event.tipo == TIPO_EVENTO_BUTTON_ON){
        CONSUMO_TOTAL = 0;
        ALARMA_1_USADA = false;
        ALARMA_2_USADA = false;
        ALARMA_3_USADA = false;
        ALARMA_LIMITE_USADA = false;
        debug_print("[FSM] Consumo reseteado");
        ESTADO_ACTUAL = ESTADO_IDLE;
        break;
      }
      display.Write("Presione boton", "para reiniciar");
      break;
  }
}

stEvento SmartWater :: GetEvent(){
  stEvento event;
  long now = millis();
  
  CONSUMO_TOTAL += flowmeter.GetConsumptionSinceLastMeasurement();
  
  // Display consumption
  if ((now - TIEMPO_ULTIMO_LOG) > INTERVALO_LOG_MS){
    TIEMPO_ULTIMO_LOG = now;
    char buffer[32];  
    snprintf(buffer, sizeof(buffer), "%.2f [L]", CONSUMO_TOTAL);
    display.Write("Consumo total:", buffer);
  }
  
  // Round-robin para chequear eventos
  if ((now - TIEMPO_ULTIMO_EVENTO) > INTERVALO_DETECCION_MS) {
    TIEMPO_ULTIMO_EVENTO = now;

    // Intentar detectar el evento en el índice actual
    if (EVENTOS_POSIBLES[indice_evento](this, &event)) {
      // Si se detectó un evento, enviarlo a la cola
      indice_evento = (indice_evento + 1) % CANT_EVENTOS_POSIBLES;
      return event;
    }
    // Avanzar al siguiente evento
    indice_evento = (indice_evento + 1) % CANT_EVENTOS_POSIBLES;
  }
  
  // Enviar evento CONTINUE si no hay otros eventos
  event.tipo = TIPO_EVENTO_CONTINUE;
  return event;
}

const char* SmartWater :: GetLog(){
    snprintf(buffer, sizeof(buffer), "Consumo total: %.2f [L]", CONSUMO_TOTAL);
    const char* mensaje = buffer;
    return mensaje;
}

bool SmartWater :: detectar_button_on(stEvento* e){
   if (button.IsPushed()) { 
    e->tipo = TIPO_EVENTO_BUTTON_ON;
    return true;
  }
  return false;
}

bool SmartWater :: detectar_consumo_umbral1(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_IDLE || 
      ESTADO_ACTUAL == ESTADO_APERTURA_ELECTROVALVULA) 
  {
    if (!ALARMA_1_USADA && CONSUMO_TOTAL >= UMBRAL_CONSUMO_1 && CONSUMO_TOTAL < UMBRAL_CONSUMO_2) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL1;
      ALARMA_1_USADA = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detectar_consumo_umbral2(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_IDLE || 
      ESTADO_ACTUAL == ESTADO_APERTURA_ELECTROVALVULA ||   
      ESTADO_ACTUAL == ESTADO_ALARMA1) 
  {
    if (!ALARMA_2_USADA && CONSUMO_TOTAL >= UMBRAL_CONSUMO_2 && CONSUMO_TOTAL < UMBRAL_CONSUMO_3) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL2;
      ALARMA_1_USADA = true;
      ALARMA_2_USADA = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detectar_consumo_umbral3(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_IDLE ||
      ESTADO_ACTUAL == ESTADO_APERTURA_ELECTROVALVULA ||     
      ESTADO_ACTUAL == ESTADO_ALARMA1 || 
      ESTADO_ACTUAL == ESTADO_ALARMA2) 
  {
    if (!ALARMA_3_USADA && CONSUMO_TOTAL >= UMBRAL_CONSUMO_3 && CONSUMO_TOTAL < UMBRAL_LIMITE) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL3;
      ALARMA_1_USADA = true;
      ALARMA_2_USADA = true;
      ALARMA_3_USADA = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detectar_consumo_limite(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_IDLE || 
      ESTADO_ACTUAL == ESTADO_APERTURA_ELECTROVALVULA || 
      ESTADO_ACTUAL == ESTADO_ALARMA1 || 
      ESTADO_ACTUAL == ESTADO_ALARMA2 || 
      ESTADO_ACTUAL == ESTADO_ALARMA3) 
  {
    if (!ALARMA_LIMITE_USADA && CONSUMO_TOTAL >= UMBRAL_LIMITE) {
      e->tipo = TIPO_EVENTO_CONSUMO_LIMITE;
      ALARMA_1_USADA = true;
      ALARMA_2_USADA = true;
      ALARMA_3_USADA = true;
      ALARMA_LIMITE_USADA = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detectar_timeout_alarma(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_ALARMA1 || 
      ESTADO_ACTUAL == ESTADO_ALARMA2 || 
      ESTADO_ACTUAL == ESTADO_ALARMA3 || 
      ESTADO_ACTUAL == ESTADO_LIMITE) 
  {
    if (millis() - TIEMPO_INICIO_ALARMA > INTERVALO_DURACION_ALARMA) {
      e->tipo = TIPO_EVENTO_TIMEOUT_ALARMA;
      return true;
    }
  }
  return false;
}