package com.example.aguasmart;
public interface MqttListener {
    void onConnected();
    void onMessageReceived(String topic, String message);
}