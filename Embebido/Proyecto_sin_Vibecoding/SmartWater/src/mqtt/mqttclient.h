#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <PubSubClient.h>

void initMQTT(
    PubSubClient *client, 
    const char * server, 
    int port, 
    const char * subscription_topic, 
    void (*callback)(char*, u_int8_t*, unsigned int)
);
void connectMQTT(
    PubSubClient *client, 
    const char * server, 
    int port, 
    const char * subscription_topic
);

#endif