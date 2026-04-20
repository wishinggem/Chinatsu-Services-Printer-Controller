#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class HomeAssistant {
public:
    HomeAssistant();
    void begin(AsyncWebServer* server);
    void loop();
    void sendData();
    void dumpPayload();

private:
    unsigned long _lastSendTime;
    String buildPayload();
};