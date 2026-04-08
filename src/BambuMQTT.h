#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Config.h"

class BambuMQTT {
public:
    BambuMQTT();
    void begin(String serial, String userId, String cloudToken);
    void loop();
    void requestStatusUpdate();
    void sendAMSCommand(int amsId, int slotId, String material, String color);
    void sendGcode(String gcode);
    
    void pausePrint();
    void resumePrint();
    void stopPrint();
    void setChamberLight(bool on);

private:
    WiFiClientSecure _secureClient;
    PubSubClient _mqtt;
    String _serial;
    String _topicPub;
    String _topicSub;
    unsigned long _lastPing; // Track heartbeat

    void reconnect();
    void parseStatusPayload(byte* payload, unsigned int length);
    static void callback(char* topic, byte* payload, unsigned int length);
};