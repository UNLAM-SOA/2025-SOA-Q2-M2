#ifndef SMARTWATER_H
#define SMARTWATER_H

#include "button.h"
#include "buzzer.h"
#include "flowmeter.h"
#include "relay.h"
#include "lcddisplay.h"

// === Types & Structs ===
enum event_type_t {
  EVENT_TYPE_CONTINUE,
  EVENT_TYPE_BUTTON_PUSH,
  EVENT_TYPE_VALVE_DEACTIVATE,
  EVENT_TYPE_ALARM_TIMEOUT,
  EVENT_TYPE_CONSUMPTION_THRESHOLD_1,
  EVENT_TYPE_CONSUMPTION_THRESHOLD_2,
  EVENT_TYPE_CONSUMPTION_THRESHOLD_3,
  EVENT_TYPE_CONSUMPTION_THRESHOLD_LIMIT
};

enum state_t {
  STATE_IDLE,
  STATE_VALVE_OPEN,
  STATE_ALARM_1,
  STATE_ALARM_2,
  STATE_ALARM_3,
  STATE_LIMIT,
  STATE_FINISH
};

struct stEvent {
  event_type_t type;
};

class SmartWater {
    private:
        state_t CURRENT_STATE = STATE_IDLE;
        Button button;
        Buzzer buzzer;
        FlowMeter flowmeter;
        Relay valve;
        LCDDisplay display;

        float TOTAL_CONSUMPTION = 0.0;
        long ALARM_START_TIME = 0;
        long LAST_EVENT_TIME = 0;
        long LAST_LOG_TIME = 0;
        long DETECTION_INTERVAL_MS;
        long LOG_INTERVAL_MS = 1000; 
        long ALARM_DURATION_MS = 0;

        float CONSUMPTION_THRESHOLD_1;
        float CONSUMPTION_THRESHOLD_2;
        float CONSUMPTION_THRESHOLD_3;
        float LIMIT_THRESHOLD;

        bool ALARM_1_USED = false;
        bool ALARM_2_USED = false;
        bool ALARM_3_USED = false;
        bool LIMIT_ALARM_USED = false;

        short event_index = 0; 

        char buffer[32];

    public:
        SmartWater();
        SmartWater(
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
        );
        void FSM(stEvent event);
        stEvent GetEvent();
        float GetConsumption();
        const char* GetValveState();

        bool detect_button_on(stEvent* e);
        bool detect_consumption_threshold_1(stEvent* e);
        bool detect_consumption_threshold_2(stEvent* e);
        bool detect_consumption_threshold_3(stEvent* e);
        bool detect_limit_consumption(stEvent* e);
        bool detect_alarm_timeout(stEvent* e);
};

typedef bool (*func_evento_t)(SmartWater*, stEvent*);

bool detect_button_on(SmartWater* s, stEvent* e);
bool detect_consumption_threshold_1(SmartWater* s, stEvent* e);
bool detect_consumption_threshold_2(SmartWater* s, stEvent* e);
bool detect_consumption_threshold_3(SmartWater* s, stEvent* e);
bool detect_limit_consumption(SmartWater* s, stEvent* e);
bool detect_alarm_timeout(SmartWater* s, stEvent* e);

#endif