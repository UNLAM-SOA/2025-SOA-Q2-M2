#ifndef CONFIGURATIONS_H
#define CONFIGURATIONS_H

// ======== WiFi ========
#define WIFI_SSID "TeleCentro-b0f8"
#define WIFI_PASSWORD "GTZFTYZMKNMM"

// ======== MQTT ========
#define MQTT_SERVER "broker.hivemq.com"   // Public Broker
#define MQTT_PORT 1883
#define MQTT_TOPIC_CONSUMPTION_LOG "aguasmart/consume"  // Publish consumption Topic
#define MQTT_TOPIC_VALVE_STATE_LOG "aguasmart/valve/state"  // Publish valve state Topic
#define MQTT_TOPIC_VALVE_CMD "aguasmart/valve/cmd"   // Command valve Topic  

// ======== SYSTEM ========
#define SERIAL_MONITOR_BAUD_RATE 115200
#define PIN_DIGITAL_BUTTON 19
#define PIN_DIGITAL_FLOWMETER 5
#define PIN_PWM_BUZZER 4
#define PIN_DIGITAL_VALVE_RELAY 16
#define MAX_CAPACITY 10.0
#define CONSUMPTION_THRESHOLD_1 MAX_CAPACITY * 0.25
#define CONSUMPTION_THRESHOLD_2 MAX_CAPACITY * 0.50
#define CONSUMPTION_THRESHOLD_3 MAX_CAPACITY * 0.75
#define DETECTION_INTERVAL_MS 50
#define ALARM_DURATION_MS 2000
#define LOG_TIME_MS 2500

// ======== FREERTOS ========
#define QUEUE_SIZE        10     // Events Queue Size
#define STACK_SIZE        8192   // Tasks Stack Size
#define TASKS_PRIORITY    1      // Tasks priority
#define TASKS_INTERVAL    10     // Tasks interval (ms)

#endif