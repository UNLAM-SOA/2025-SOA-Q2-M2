#include "utils/utils.h"
#include "smartwater.h"
#include <Arduino.h>

func_evento_t EVENTOS_POSIBLES[6] = {
  detectar_button_on,
  detectar_button_off,
  detectar_consumo_umbral1,
  detectar_consumo_umbral2,
  detectar_consumo_umbral3,
  detectar_consumo_limite
};

bool detectar_button_on(SmartWater* s, stEvento* e) {
  return s->detectar_button_on(e);
};

bool detectar_button_off(SmartWater* s, stEvento* e) {
  return s->detectar_button_off(e);
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

SmartWater :: SmartWater(){}

SmartWater :: SmartWater(
    const int PIN_DIGITAL_PULSADOR,
    const int PIN_DIGITAL_CAUDALIMETRO,
    const int PIN_PWM_BUZZER,
    const int PIN_DIGITAL_RELAY_ELECTROVALVULA,
    const float UMBRAL_CONSUMO_1,
    const float UMBRAL_CONSUMO_2,
    const float UMBRAL_CONSUMO_3,
    const char* WIFI_SSID,
    const char* WIFI_PASSWORD,
    const char* MQTT_TOPIC_NAME,
    const char* MQTT_SERVER,
    const int MQTT_PORT,
    const long INTERVALO_DETECCION_MS
){
    button = Button(PIN_DIGITAL_PULSADOR);

    buzzer = Buzzer(PIN_PWM_BUZZER);

    flowmeter = FlowMeter(PIN_DIGITAL_CAUDALIMETRO);

    valve = Relay(PIN_DIGITAL_RELAY_ELECTROVALVULA);

    display = LCDDisplay();

    this->INTERVALO_DETECCION_MS = INTERVALO_DETECCION_MS;

    debug_print("==============================================");
    debug_print("[SISTEMA] SmartWater inicializado correctamente");
    debug_print("==============================================");
    display.Write("SmartWater", "Grupo M2");
}

// === Máquina de estados ===
void SmartWater :: FSM(stEvento event){
  switch (ESTADO_ACTUAL) {
    case ESTADO_IDLE:
      if (event.tipo == TIPO_EVENTO_BUTTON_ON) {
        debug_print("[FSM] IDLE -> APERTURA_ELECTROVALVULA");
        valve.activate();
        // reproducir_alarma(FREC_UMBRAL1); // ESTO NO VA
        // tiempo_inicio_valvula = millis();
        // timeout_valvula_habilitado = true;
        ESTADO_ACTUAL = ESTADO_APERTURA_ELECTROVALVULA;
      }
      break;
      
    case ESTADO_APERTURA_ELECTROVALVULA:
      switch (event.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] APERTURA_ELECTROVALVULA -> IDLE");
          valve.deactivate();
          // detener_alarma(); ESTO NO VA
          // debug_print("[FSM] DETENIENDO ALARMA"); ESTO NO VA

          // timeout_valvula_habilitado = false;
          ESTADO_ACTUAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_VALVULA: // CHECKEAR
          debug_print("[FSM] APERTURA_ELECTROVALVULA -> NORMAL");
          ESTADO_ACTUAL = ESTADO_NORMAL;
          break;
      }
      break;
      
    case ESTADO_NORMAL:
      switch (event.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          valve.deactivate();
          ESTADO_ACTUAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL1:
          debug_print("[FSM] NORMAL -> ALARMA1");
          // reproducir_alarma(FREC_UMBRAL1);
          // tiempo_inicio_alarma = millis();
          // timeout_alarma_habilitado = true;
          // NIVEL_ALARMA_ACTUAL = FREC_UMBRAL1;
          buzzer.PlayAlarm(1);
          ESTADO_ACTUAL = ESTADO_ALARMA1;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL2:
          debug_print("[FSM] NORMAL -> ALARMA2");
          // reproducir_alarma(FREC_UMBRAL2);
          // tiempo_inicio_alarma = millis();
          // timeout_alarma_habilitado = true;
          // NIVEL_ALARMA_ACTUAL = FREC_UMBRAL2;
          buzzer.PlayAlarm(2);
          ESTADO_ACTUAL = ESTADO_ALARMA2;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL3:
          debug_print("[FSM] NORMAL -> ALARMA3");
          // reproducir_alarma(FREC_UMBRAL3);
          // tiempo_inicio_alarma = millis();
          // timeout_alarma_habilitado = true;
          // NIVEL_ALARMA_ACTUAL = FREC_UMBRAL3;
          buzzer.PlayAlarm(3);
          ESTADO_ACTUAL = ESTADO_ALARMA3;
          break;
        case TIPO_EVENTO_CONSUMO_LIMITE:
          debug_print("[FSM] NORMAL -> ALARMA4");
          // reproducir_alarma(FREC_LIMITE);
          // tiempo_inicio_alarma = millis();
          // timeout_alarma_habilitado = true;
          // NIVEL_ALARMA_ACTUAL = FREC_LIMITE;
          buzzer.PlayAlarm(4);
          ESTADO_ACTUAL = ESTADO_LIMITE;
          break;
      }
      break;
      
    case ESTADO_ALARMA1:
      switch (event.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          valve.deactivate();
          ESTADO_ACTUAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA1 -> NORMAL");
          buzzer.StopAlarm();
          ESTADO_ACTUAL = ESTADO_NORMAL;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL2:
          debug_print("[FSM] ALARMA1 -> ALARMA2");
          buzzer.StopAlarm();
          // reproducir_alarma(FREC_UMBRAL2);
          // tiempo_inicio_alarma = millis();
          // timeout_alarma_habilitado = true;
          // NIVEL_ALARMA_ACTUAL = FREC_UMBRAL2;
          buzzer.PlayAlarm(2);
          ESTADO_ACTUAL = ESTADO_ALARMA2;
          break;
      }
      break;
      
    case ESTADO_ALARMA2:
      switch (event.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          valve.deactivate();
          ESTADO_ACTUAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA2 -> NORMAL");
          buzzer.StopAlarm();
          ESTADO_ACTUAL = ESTADO_NORMAL;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL3:
          debug_print("[FSM] ALARMA2 -> ALARMA3");
          buzzer.StopAlarm();
          // reproducir_alarma(FREC_UMBRAL3);
          // tiempo_inicio_alarma = millis();
          // timeout_alarma_habilitado = true;
          // NIVEL_ALARMA_ACTUAL = FREC_UMBRAL3;
          buzzer.PlayAlarm(3);
          ESTADO_ACTUAL = ESTADO_ALARMA3;
          break;
      }
      break;
      
    case ESTADO_ALARMA3:
      switch (event.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          valve.deactivate();
          ESTADO_ACTUAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA3 -> NORMAL");
          buzzer.StopAlarm();
          ESTADO_ACTUAL = ESTADO_NORMAL;
          break;
        case TIPO_EVENTO_CONSUMO_LIMITE:
          debug_print("[FSM] ALARMA3 -> ALARMA4");
          buzzer.StopAlarm();
          // reproducir_alarma(FREC_LIMITE);
          // tiempo_inicio_alarma = millis();
          // timeout_alarma_habilitado = true;
          // NIVEL_ALARMA_ACTUAL = FREC_LIMITE;
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
        ESTADO_ACTUAL = ESTADO_IDLE;
      }
      break;
  }
}

stEvento SmartWater :: GetEvent(){
  stEvento event;
  long now = millis();
  
  float consumo = flowmeter.GetConsumptionSinceLastMeasurement();
  CONSUMO_TOTAL += consumo;
  
  // Log consumo
  if ((now - TIEMPO_ULTIMO_LOG) > INTERVALO_LOG_MS && consumo > 0){
    TIEMPO_ULTIMO_LOG = now;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Consumo total: %.2f [L]", CONSUMO_TOTAL);
    const char* mensaje = buffer;
    debug_print(mensaje);

    snprintf(buffer, sizeof(buffer), "%.2f [L]", CONSUMO_TOTAL);
    mensaje = buffer;
    display.Write("Consumo total:", buffer);
  }
  
  int MAX_EVENTOS = 6;

  // Round-robin para chequear eventos
  if ((now - TIEMPO_ULTIMO_EVENTO) > INTERVALO_DETECCION_MS) {
    TIEMPO_ULTIMO_EVENTO = now;

    // Intentar detectar el evento en el índice actual
    if (EVENTOS_POSIBLES[indice_evento](this, &event)) {
      // debug_print("[EVENTO] Evento detectado: " + event.tipo);
      // Si se detectó un evento, enviarlo a la cola
      indice_evento = (indice_evento + 1) % MAX_EVENTOS;
      return event;
    }
    // Avanzar al siguiente evento
    indice_evento = (indice_evento + 1) % MAX_EVENTOS;
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

// void SmartWater :: SendLog(const char* msg){
//   debug_print(msg);
//   mqttClient.Publish(msg);
// }

bool SmartWater :: detectar_button_on(stEvento* e){
   if (ESTADO_ACTUAL == ESTADO_IDLE && button.IsPushed()) { 
    debug_print("Botón presionado");
    // CHECKEAR: MANEJO DE ESTADO ACA O SOLO SWITCH?
    // PROBAR SI BOTON ASI FUNCIONA O AÑADIR LOGICA DE TOGGLE
    e->tipo = TIPO_EVENTO_BUTTON_ON;
    return true;
  }
  return false;
}

bool SmartWater :: detectar_button_off(stEvento* e){
  if ((ESTADO_ACTUAL == ESTADO_APERTURA_ELECTROVALVULA || ESTADO_ACTUAL == ESTADO_NORMAL) 
  && !button.IsPushed()) {
        debug_print("Botón presionado off");
    e->tipo = TIPO_EVENTO_BUTTON_OFF;
    return true;
  }
  return false;
}

bool SmartWater :: detectar_consumo_umbral1(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_NORMAL) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_TOTAL >= UMBRAL_CONSUMO_1 && CONSUMO_TOTAL < UMBRAL_CONSUMO_2) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL1;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detectar_consumo_umbral2(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_NORMAL || ESTADO_ACTUAL == ESTADO_ALARMA1) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_TOTAL >= UMBRAL_CONSUMO_2 && CONSUMO_TOTAL < UMBRAL_CONSUMO_3) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL2;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detectar_consumo_umbral3(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_NORMAL || ESTADO_ACTUAL == ESTADO_ALARMA1 || ESTADO_ACTUAL == ESTADO_ALARMA2) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_TOTAL >= UMBRAL_CONSUMO_3) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL3;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detectar_consumo_limite(stEvento* e){
  if (ESTADO_ACTUAL == ESTADO_NORMAL || ESTADO_ACTUAL == ESTADO_ALARMA1 || 
      ESTADO_ACTUAL == ESTADO_ALARMA2 || ESTADO_ACTUAL == ESTADO_ALARMA3) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_TOTAL >= UMBRAL_CONSUMO_3) {
      e->tipo = TIPO_EVENTO_CONSUMO_LIMITE;
      return true;
    }
  }
  return false;
}