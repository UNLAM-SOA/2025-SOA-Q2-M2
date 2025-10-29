// #include "mqttclient.h"
// #include "utils/utils.h"

// WiFiClient espClient;
// PubSubClient client(espClient);

// MQTTClient :: MQTTClient(){}

// MQTTClient::MQTTClient(const char* MQTT_TOPIC_NAME, const char* MQTT_SERVER, const int MQTT_PORT) {
//     client.setServer(MQTT_SERVER, MQTT_PORT);
// }

// void MQTTClient :: Connect() {
//   while (!client.connected()) {
//     debug_print("conectando MQTT...");
//     if (client.connect("ESP32Client")) {
//       debug_print(" conectado!");
//     } else {
//       debug_print("error conectando MQTT");
//     }
//   }
// }

// void MQTTClient :: Publish(const char* msg) {
//   long now = millis();
//   if ((now - TIEMPO_ULTIMO_LOG) > INTERVALO_LOG_MS){
//     if (!client.connected()){
//       this->Connect();
//     }
//     client.loop();
//     client.publish(topicName, msg);
//     TIEMPO_ULTIMO_LOG = now;
//   }
// }



#include "mqttclient.h"

