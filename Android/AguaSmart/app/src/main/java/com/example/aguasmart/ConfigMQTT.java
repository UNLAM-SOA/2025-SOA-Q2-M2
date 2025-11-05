package com.example.aguasmart;

public class ConfigMQTT {
    public static String MQTT_SERVER;
    public static String CLIENT_ID = "AguasMartClient";
    public static String TOPIC_VALVULA = "aguasmart/valvula";
    public static String TOPIC_ESTADO = "aguasmart/estado";

    public static String TOPIC = "pruebita";

    public static void useServerHiveMQ() {
        MQTT_SERVER = "tcp://broker.hivemq.com:1883";
    }
}