#include "utils/utils.h"
#include "smartwater.h"
#include <Arduino.h>

#define ACTIVE_STATE_STRING "active"
#define INACTIVE_STATE_STRING "inactive"

#define QTY_POSSIBLE_EVENTS 6
func_evento_t POSSIBLE_EVENTS[QTY_POSSIBLE_EVENTS] = {
  detect_button_on,
  detect_consumption_threshold_1,
  detect_consumption_threshold_2,
  detect_consumption_threshold_3,
  detect_limit_consumption,
  detect_alarm_timeout
};

bool detect_button_on(SmartWater* s, stEvent* e) {
  return s->detect_button_on(e);
};

bool detect_consumption_threshold_1(SmartWater* s, stEvent* e) {
  return s->detect_consumption_threshold_1(e);
};

bool detect_consumption_threshold_2(SmartWater* s, stEvent* e) {
  return s->detect_consumption_threshold_2(e);
};

bool detect_consumption_threshold_3(SmartWater* s, stEvent* e) {
  return s->detect_consumption_threshold_3(e);
};

bool detect_limit_consumption(SmartWater* s, stEvent* e) {
  return s->detect_limit_consumption(e);
};

bool detect_alarm_timeout(SmartWater* s, stEvent* e) {
  return s->detect_alarm_timeout(e);
}

SmartWater :: SmartWater(){}

SmartWater :: SmartWater(
    const int PIN_DIGITAL_BUTTON,
    const int PIN_DIGITAL_FLOWMETER,
    const int PIN_PWM_BUZZER,
    const int PIN_DIGITAL_VALVE_RELAY,
    const float CONSUMPTION_THRESHOLD_1,
    const float CONSUMPTION_THRESHOLD_2,
    const float CONSUMPTION_THRESHOLD_3,
    const float LIMIT_THRESHOLD,
    const char* MQTT_TOPIC_NAME,
    const char* MQTT_SERVER,
    const int MQTT_PORT,
    const long DETECTION_INTERVAL_MS,
    const long ALARM_DURATION_MS
){
    button = Button(PIN_DIGITAL_BUTTON);

    buzzer = Buzzer(PIN_PWM_BUZZER);

    flowmeter = FlowMeter(PIN_DIGITAL_FLOWMETER);

    valve = Relay(PIN_DIGITAL_VALVE_RELAY);

    display = LCDDisplay(500);

    this->DETECTION_INTERVAL_MS = DETECTION_INTERVAL_MS;
    this->CONSUMPTION_THRESHOLD_1 = CONSUMPTION_THRESHOLD_1;
    this->CONSUMPTION_THRESHOLD_2 = CONSUMPTION_THRESHOLD_2;
    this->CONSUMPTION_THRESHOLD_3 = CONSUMPTION_THRESHOLD_3;
    this->LIMIT_THRESHOLD = LIMIT_THRESHOLD;
    this->ALARM_DURATION_MS = ALARM_DURATION_MS;

    debug_print("==============================================");
    debug_print("[SISTEMA] SmartWater successfully initialized");
    debug_print("==============================================");
    display.Write("SmartWater", "Grupo M2");
}

// === State machine ===
void SmartWater :: FSM(stEvent event){
  switch (CURRENT_STATE) {
    case STATE_IDLE:
      switch (event.type) {
        case EVENT_TYPE_BUTTON_PUSH:
          debug_print("[FSM] IDLE -> VALVE_OPEN");
          valve.activate();
          CURRENT_STATE = STATE_VALVE_OPEN;
          break;
      }
      break;
      
    case STATE_VALVE_OPEN:
      switch (event.type) {
        case EVENT_TYPE_BUTTON_PUSH:
          debug_print("[FSM] VALVE_OPEN -> IDLE");
          valve.deactivate();
          CURRENT_STATE = STATE_IDLE;
          break;

        case EVENT_TYPE_VALVE_DEACTIVATE:
          debug_print("[FSM] VALVE_OPEN -> NORMAL");
          valve.deactivate();
          CURRENT_STATE = STATE_IDLE;
          break;

        case EVENT_TYPE_CONSUMPTION_THRESHOLD_1:
          debug_print("[FSM] NORMAL -> ALARM_1");
          buzzer.PlayAlarm(1);
          CURRENT_STATE = STATE_ALARM_1;
          ALARM_START_TIME = millis();
          break;

        case EVENT_TYPE_CONSUMPTION_THRESHOLD_2:
          debug_print("[FSM] NORMAL -> ALARM_2");
          buzzer.PlayAlarm(2);
          CURRENT_STATE = STATE_ALARM_2;
          ALARM_START_TIME = millis();
          break;

        case EVENT_TYPE_CONSUMPTION_THRESHOLD_3:
          debug_print("[FSM] NORMAL -> ALARM_3");
          buzzer.PlayAlarm(3);
          CURRENT_STATE = STATE_ALARM_3;
          ALARM_START_TIME = millis();
          break;

        case EVENT_TYPE_CONSUMPTION_THRESHOLD_LIMIT:
          debug_print("[FSM] NORMAL -> ALARM_4");
          buzzer.PlayAlarm(4);
          CURRENT_STATE = STATE_LIMIT;
          ALARM_START_TIME = millis();
          break;
      }
      break;
            
    case STATE_ALARM_1:
      switch (event.type) {
        case EVENT_TYPE_ALARM_TIMEOUT:
          debug_print("[FSM] ALARM_1 -> NORMAL");
          buzzer.StopAlarm();
          CURRENT_STATE = STATE_VALVE_OPEN;
          break;

        case EVENT_TYPE_CONSUMPTION_THRESHOLD_2:
          debug_print("[FSM] ALARM_1 -> ALARM_2");
          buzzer.PlayAlarm(2);
          CURRENT_STATE = STATE_ALARM_2;
          break;
        
        case EVENT_TYPE_VALVE_DEACTIVATE:
          debug_print("[FSM] ALARM_1 -> NORMAL");
          valve.deactivate();
          CURRENT_STATE = STATE_IDLE;
          break;
      }
      break;
      
    case STATE_ALARM_2:
      switch (event.type) {
        case EVENT_TYPE_ALARM_TIMEOUT:
          debug_print("[FSM] ALARM_2 -> NORMAL");
          buzzer.StopAlarm();
          CURRENT_STATE = STATE_VALVE_OPEN;
          break;

        case EVENT_TYPE_CONSUMPTION_THRESHOLD_3:
          debug_print("[FSM] ALARM_2 -> ALARM_3");
          buzzer.PlayAlarm(3);
          CURRENT_STATE = STATE_ALARM_3;
          break;
        
        case EVENT_TYPE_VALVE_DEACTIVATE:
          debug_print("[FSM] ALARM_2 -> NORMAL");
          valve.deactivate();
          CURRENT_STATE = STATE_IDLE;
          break;
      }
      break;
      
    case STATE_ALARM_3:
      switch (event.type) {
        case EVENT_TYPE_ALARM_TIMEOUT:
          debug_print("[FSM] ALARM_3 -> NORMAL");
          buzzer.StopAlarm();
          CURRENT_STATE = STATE_VALVE_OPEN;
          break;

        case EVENT_TYPE_CONSUMPTION_THRESHOLD_LIMIT:
          debug_print("[FSM] ALARM_3 -> ALARM_4");
          buzzer.PlayAlarm(4);
          CURRENT_STATE = STATE_LIMIT;
          break;

        case EVENT_TYPE_VALVE_DEACTIVATE:
          debug_print("[FSM] ALARM_3 -> NORMAL");
          valve.deactivate();
          CURRENT_STATE = STATE_IDLE;
          break;
      }
      break;
      
    case STATE_LIMIT:
      if (event.type == EVENT_TYPE_ALARM_TIMEOUT) {
        debug_print("[FSM] ALARM_4 -> IDLE (LIMIT REACHED)");
        buzzer.StopAlarm();
        valve.deactivate();
        CURRENT_STATE = STATE_FINISH;
      }
      break;

    case STATE_FINISH:
      if (event.type == EVENT_TYPE_BUTTON_PUSH){
        TOTAL_CONSUMPTION = 0;
        ALARM_1_USED = false;
        ALARM_2_USED = false;
        ALARM_3_USED = false;
        LIMIT_ALARM_USED = false;
        debug_print("[FSM] Consumo reseteado");
        CURRENT_STATE = STATE_IDLE;
        break;
      }
      display.Write("Presione boton", "para reiniciar");
      break;
  }
}

stEvent SmartWater :: GetEvent(){
  stEvent event;
  long now = millis();
  
  TOTAL_CONSUMPTION += flowmeter.GetConsumptionSinceLastMeasurement();
  
  // Display consumption.
  if ((now - LAST_LOG_TIME) > LOG_INTERVAL_MS){
    LAST_LOG_TIME = now;
    char buffer[32];  
    snprintf(buffer, sizeof(buffer), "%.2f [L]", TOTAL_CONSUMPTION);
    display.Write("Consumo total:", buffer);
  }
  
  // Round-robin to check events.
  if ((now - LAST_EVENT_TIME) > DETECTION_INTERVAL_MS) {
    LAST_EVENT_TIME = now;

    // Try to detect event on current index.
    if (POSSIBLE_EVENTS[event_index](this, &event)) {
      // If event detected, send it to queue
      event_index = (event_index + 1) % QTY_POSSIBLE_EVENTS;
      return event;
    }
    // Continue to next event.
    event_index = (event_index + 1) % QTY_POSSIBLE_EVENTS;
  }
  
  // Send CONTINUE event if there are no other events.
  event.type = EVENT_TYPE_CONTINUE;
  return event;
}

float SmartWater :: GetConsumption(){
  return this->TOTAL_CONSUMPTION;
}
        
const char* SmartWater :: GetValveState(){
  snprintf(buffer, sizeof(buffer), INACTIVE_STATE_STRING);
  if (this->valve.IsActive()){
    snprintf(buffer, sizeof(buffer), ACTIVE_STATE_STRING);
  }
  const char* mensaje = buffer;
  return mensaje;
}

bool SmartWater :: detect_button_on(stEvent* e){
   if (button.IsPushed()) { 
    e->type = EVENT_TYPE_BUTTON_PUSH;
    return true;
  }
  return false;
}

bool SmartWater :: detect_consumption_threshold_1(stEvent* e){
  if (CURRENT_STATE == STATE_IDLE || 
      CURRENT_STATE == STATE_VALVE_OPEN) 
  {
    if (!ALARM_1_USED && TOTAL_CONSUMPTION >= CONSUMPTION_THRESHOLD_1 && TOTAL_CONSUMPTION < CONSUMPTION_THRESHOLD_2) {
      e->type = EVENT_TYPE_CONSUMPTION_THRESHOLD_1;
      ALARM_1_USED = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detect_consumption_threshold_2(stEvent* e){
  if (CURRENT_STATE == STATE_IDLE || 
      CURRENT_STATE == STATE_VALVE_OPEN ||   
      CURRENT_STATE == STATE_ALARM_1) 
  {
    if (!ALARM_2_USED && TOTAL_CONSUMPTION >= CONSUMPTION_THRESHOLD_2 && TOTAL_CONSUMPTION < CONSUMPTION_THRESHOLD_3) {
      e->type = EVENT_TYPE_CONSUMPTION_THRESHOLD_2;
      ALARM_1_USED = true;
      ALARM_2_USED = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detect_consumption_threshold_3(stEvent* e){
  if (CURRENT_STATE == STATE_IDLE ||
      CURRENT_STATE == STATE_VALVE_OPEN ||     
      CURRENT_STATE == STATE_ALARM_1 || 
      CURRENT_STATE == STATE_ALARM_2) 
  {
    if (!ALARM_3_USED && TOTAL_CONSUMPTION >= CONSUMPTION_THRESHOLD_3 && TOTAL_CONSUMPTION < LIMIT_THRESHOLD) {
      e->type = EVENT_TYPE_CONSUMPTION_THRESHOLD_3;
      ALARM_1_USED = true;
      ALARM_2_USED = true;
      ALARM_3_USED = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detect_limit_consumption(stEvent* e){
  if (CURRENT_STATE == STATE_IDLE || 
      CURRENT_STATE == STATE_VALVE_OPEN || 
      CURRENT_STATE == STATE_ALARM_1 || 
      CURRENT_STATE == STATE_ALARM_2 || 
      CURRENT_STATE == STATE_ALARM_3) 
  {
    if (!LIMIT_ALARM_USED && TOTAL_CONSUMPTION >= LIMIT_THRESHOLD) {
      e->type = EVENT_TYPE_CONSUMPTION_THRESHOLD_LIMIT;
      ALARM_1_USED = true;
      ALARM_2_USED = true;
      ALARM_3_USED = true;
      LIMIT_ALARM_USED = true;
      return true;
    }
  }
  return false;
}

bool SmartWater :: detect_alarm_timeout(stEvent* e){
  if (CURRENT_STATE == STATE_ALARM_1 || 
      CURRENT_STATE == STATE_ALARM_2 || 
      CURRENT_STATE == STATE_ALARM_3 || 
      CURRENT_STATE == STATE_LIMIT) 
  {
    if (millis() - ALARM_START_TIME > ALARM_DURATION_MS) {
      e->type = EVENT_TYPE_ALARM_TIMEOUT;
      return true;
    }
  }
  return false;
}