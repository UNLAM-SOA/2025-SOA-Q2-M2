#include "mqttclient.h"
#include "utils/utils.h"

void initMQTT(
    PubSubClient *client, 
    const char * server, 
    int port, 
    const char * subscription_topic, 
    void (*callback)(char*, u_int8_t*, unsigned int)
) {
    client->setServer(server, port);
    client->setCallback(callback);
    client->setKeepAlive(60);
    client->setSocketTimeout(60);
    client->setBufferSize(512);
    connectMQTT(client, server, port, subscription_topic);
}

void connectMQTT(
    PubSubClient *client, 
    const char * server, 
    int port, 
    const char * subscription_topic
) {
  while (!client->connected()) {
    debug_printf("[MQTT] Connecting to %s:%d...\n", server, port);
    
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "ESP32Client-%lu", millis());

    if (client->connect(clientId)) {
      debug_printf("[MQTT] Connected to broker MQTT!\n");
      if (client->subscribe(subscription_topic)) {
        debug_printf("[MQTT] Successfull subscription to %s\n", subscription_topic);
      } else {
        debug_printf("[MQTT] ERROR: Subscription failed to %s\n", subscription_topic);
      }
    } else {
      debug_printf("[MQTT] ERROR: Connection failed. rc=%d\n", client->state());
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}