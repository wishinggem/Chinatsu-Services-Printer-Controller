#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include <vector>

struct HMSError {
    String code;
    unsigned long timestamp;
    bool isNew;
};

struct HMSDictEntry {
    String shortDesc;
    String longDesc;
};

extern std::vector<HMSError> hmsHistory;
extern bool hasNewHmsError;
extern bool chamberLightOn;
HMSDictEntry getHMSEntry(String code);

extern int printLayer;
extern int printTotalLayers;
extern int printRemainingTime;
extern int printSpeedLevel;
extern int fanPartSpeed;
extern int fanAuxSpeed;
extern int fanChamberSpeed;
extern String printGcodeFile;
extern bool hasAutoSwitchedToPrint;

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
    unsigned long _lastUpdateRx; // Track heartbeat

    void reconnect();
    void parseStatusPayload(byte* payload, unsigned int length);
    static void callback(char* topic, byte* payload, unsigned int length);
};