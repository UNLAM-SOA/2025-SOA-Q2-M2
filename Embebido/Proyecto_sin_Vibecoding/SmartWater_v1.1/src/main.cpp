#include "smartwater/smartwater.h"
#include "configurations.h"
#include "utils/utils.h"
#include "wifi/wifi.h"
#include "mqtt/mqttclient.h"
#include <PubSubClient.h>

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

// Function that is called when MQTT receives a message from a subscribed topic
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] MSG received in topic: %s\n", topic);
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("[MQTT] Payload: %s\n", msg.c_str());

  stEvento event;

  if (msg == "button_on") {
    event.tipo = TIPO_EVENTO_BUTTON_ON;
  } else {
    Serial.println("[MQTT] MSG not recognized, ignored");
    return;
  }

  xQueueSend(queueEvents, &event, portMAX_DELAY);
}

// === FreeRTOS - Loop ===
void vLoopTask(void *pvParameters) {
  while (1) {
    stEvento event;
    xQueueReceive(queueEvents, &event, portMAX_DELAY);
    smartWater->FSM(event);
  }
}

// === FreeRTOS - Event Handler ===
void vGetNewEventTask(void *pvParameters) {
  while (1) {
    stEvento event = smartWater->GetEvent();
    xQueueSend(queueEvents, &event, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(TASKS_INTERVAL));
  }
}

// === FreeRTOS - MQTT Events Handler ===
void vMqttTask(void* pvParameters) {
  connectWiFi(WIFI_SSID, WIFI_PASSWORD);
  initMQTT(&mqttClient, MQTT_SERVER, MQTT_PORT, MQTT_TOPIC_CMD, mqttCallback);
  
  char logMsg[128];
  while (1) {
    mqttClient.loop();
    if (!mqttClient.connected()) {
      Serial.println("[MQTT] Lost connection, retrying connection...");
      connectMQTT(&mqttClient, MQTT_SERVER, MQTT_PORT, MQTT_TOPIC_CMD);
      vTaskDelay(pdMS_TO_TICKS(5000)); // Waits 5s before retrying reconnect
    }

    // If there are logs in the queue, they are sent to the MQTT Topic
    if (xQueueReceive(queueMQTT, logMsg, 0) == pdTRUE) {
      mqttClient.publish(MQTT_TOPIC_LOG, logMsg);
      Serial.printf("[MQTT] MSG Sent: %s\n", logMsg);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// === FreeRTOS - Log Events Handler ===
void vLogTask(void* pvParameters) {
  while (true) {
    char log[64];
    snprintf(log, sizeof(log), smartWater->GetLog());

    // Send Log to MQTT Queue
    if (xQueueSend(queueMQTT, log, 0) != pdTRUE) {
      Serial.println("[WARN] MQTT Queue is full, discarding log");
    } 

    vTaskDelay(pdMS_TO_TICKS(LOG_TIME_MS)); 
  }
}

void setup() {
  Serial.begin(SERIAL_MONITOR_BAUD_RATE);
  
  // System Main class init
  smartWater = new SmartWater(
    PIN_DIGITAL_PULSADOR,
    PIN_DIGITAL_CAUDALIMETRO,
    PIN_PWM_BUZZER,
    PIN_DIGITAL_RELAY_ELECTROVALVULA,
    UMBRAL_CONSUMO_1,
    UMBRAL_CONSUMO_2,
    UMBRAL_CONSUMO_3,
    CAPACIDAD_MAXIMA,
    MQTT_TOPIC_LOG,
    MQTT_SERVER,
    MQTT_PORT,
    INTERVALO_DETECCION_MS,
    INTERVALO_DURACION_ALARMA_MS
  );
  
  // FreeRTOS - Init Queues
  queueEvents = xQueueCreate(QUEUE_SIZE, sizeof(stEvento));
  queueMQTT = xQueueCreate(QUEUE_SIZE, sizeof(char[64]));
  
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