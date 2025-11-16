#include "smartwater/smartwater.h"
#include "configurations.h"
#include "utils/utils.h"
#include "wifi/wifi.h"
#include "mqtt/mqttclient.h"
#include <PubSubClient.h>
// This is the expected string from the CMD topic to
// act like as a BUTTON_PUSH event.
#define CMD_MSG_BUTTON_PUSH "button_push"

// === FreeRTOS ===
// Queues
QueueHandle_t queueEvents;
QueueHandle_t queueMQTT;
// TaskHandlers
TaskHandle_t loopTaskHandler;
TaskHandle_t loopNewEventHandler;
TaskHandle_t loopLogHandler;
TaskHandle_t loopMQTTHandler;
// ====================================
// WIFI & MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
// ====================================
// Main Class
SmartWater* smartWater;
// ====================================
// Msg struct to send to MQTT
struct TopicLogMSG {
  float consumption;
  char valveState[32];
};

// Function that is called when MQTT receives a message from a subscribed topic
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] MSG received in topic: %s\n", topic);
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("[MQTT] Payload: %s\n", msg.c_str());

  stEvent event;

  if (msg == CMD_MSG_BUTTON_PUSH) {
    event.type = EVENT_TYPE_BUTTON_PUSH;
  } else {
    Serial.println("[MQTT] MSG not recognized, ignored");
    return;
  }

  xQueueSend(queueEvents, &event, portMAX_DELAY);
}

// === FreeRTOS - Loop ===
void vLoopTask(void *pvParameters) {
  while (1) {
    stEvent event;
    xQueueReceive(queueEvents, &event, portMAX_DELAY);
    smartWater->FSM(event);
  }
}

// === FreeRTOS - Event Handler ===
void vGetNewEventTask(void *pvParameters) {
  while (1) {
    stEvent event = smartWater->GetEvent();
    xQueueSend(queueEvents, &event, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(TASKS_INTERVAL));
  }
}

// === FreeRTOS - MQTT Events Handler ===
void vMqttTask(void* pvParameters) {
  connectWiFi(WIFI_SSID, WIFI_PASSWORD);
  initMQTT(&mqttClient, MQTT_SERVER, MQTT_PORT, MQTT_TOPIC_VALVE_CMD, mqttCallback);
  
  // char logMsg[128];
  TopicLogMSG logMsg;
  while (1) {
    mqttClient.loop();
    if (!mqttClient.connected()) {
      Serial.println("[MQTT] Lost connection, retrying connection...");
      connectMQTT(&mqttClient, MQTT_SERVER, MQTT_PORT, MQTT_TOPIC_VALVE_CMD);
      vTaskDelay(pdMS_TO_TICKS(5000)); // Waits 5s before retrying reconnect
    }

    // If there are logs in the queue, they are sent to the MQTT Topic
    if (xQueueReceive(queueMQTT, &logMsg, 0) == pdTRUE) {
      char buffer[20];
      snprintf(buffer, sizeof(buffer), "%.2f", logMsg.consumption);
      const char* consumption = buffer;

      mqttClient.publish(MQTT_TOPIC_CONSUMPTION_LOG, consumption);
      Serial.printf("[MQTT] MSG Sent to topic %s: %s\n", MQTT_TOPIC_CONSUMPTION_LOG, consumption);
      mqttClient.publish(MQTT_TOPIC_VALVE_STATE_LOG, logMsg.valveState);
      Serial.printf("[MQTT] MSG Sent to topic %s: %s\n", MQTT_TOPIC_VALVE_STATE_LOG, logMsg.valveState);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// === FreeRTOS - Log Events Handler ===
void vLogTask(void* pvParameters) {
  while (true) {
    TopicLogMSG log;
    log.consumption = smartWater->GetConsumption();
    snprintf(log.valveState, sizeof(log.valveState), "%s", smartWater->GetValveState());

    // Send Log to MQTT Queue
    if (xQueueSend(queueMQTT, &log, 0) != pdTRUE) {
      Serial.println("[WARN] MQTT Queue is full, discarding log");
    } 

    vTaskDelay(pdMS_TO_TICKS(LOG_TIME_MS)); 
  }
}

void setup() {
  Serial.begin(SERIAL_MONITOR_BAUD_RATE);
  
  // System Main class init
  smartWater = new SmartWater(
    PIN_DIGITAL_BUTTON,
    PIN_DIGITAL_FLOWMETER,
    PIN_PWM_BUZZER,
    PIN_DIGITAL_VALVE_RELAY,
    CONSUMPTION_THRESHOLD_1,
    CONSUMPTION_THRESHOLD_2,
    CONSUMPTION_THRESHOLD_3,
    MAX_CAPACITY,
    MQTT_TOPIC_CONSUMPTION_LOG,
    MQTT_SERVER,
    MQTT_PORT,
    DETECTION_INTERVAL_MS,
    ALARM_DURATION_MS
  );
  
  // FreeRTOS - Init Queues
  queueEvents = xQueueCreate(QUEUE_SIZE, sizeof(stEvent));
  queueMQTT = xQueueCreate(QUEUE_SIZE, sizeof(TopicLogMSG));
  
  // FreeRTOS - Loop
  xTaskCreate(
    vLoopTask,
    "LoopFSM",
    STACK_SIZE,
    NULL,
    TASKS_PRIORITY,
    &loopTaskHandler
  );
  
  // FreeRTOS - Event handler
  xTaskCreate(
    vGetNewEventTask,
    "EventTask",
    STACK_SIZE,
    NULL,
    TASKS_PRIORITY,
    &loopNewEventHandler
  );

  // FreeRTOS - MQTT events handler
  xTaskCreate(
    vMqttTask, 
    "MQTT Task", 
    STACK_SIZE, 
    NULL, 
    TASKS_PRIORITY, 
    &loopMQTTHandler
  );

  // FreeRTOS - Log events
  xTaskCreate(
    vLogTask, 
    "Log Task", 
    STACK_SIZE, 
    NULL, 
    TASKS_PRIORITY, 
    &loopLogHandler
  );
}

void loop(){
  // The system loop is handled by FreeRTOS in vLoopTask
}