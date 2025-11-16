#include "wifi.h"

void connectWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  Serial.printf("[WIFI] Connecting to %s...\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Connected!");
}
