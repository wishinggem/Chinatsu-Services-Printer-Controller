#pragma once
#include "Page.h"

class PagePrintMonitor : public Page {
public:
    PagePrintMonitor(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    unsigned long lastUpdate;
    bool notificationDrawn = false;
    unsigned long notificationTime = 0;

    String lastStatus;
    int lastLayer;
    int lastTotalLayers;
    int lastTime;
    int lastSpeed;
    int lastPart;
    int lastAux;
    int lastChamber;
    int lastNoz;
    int lastBed;
    int lastNozTarget;
    int lastBedTarget;
    bool lastLightState;
    

    void drawLiveData();
    void drawArcGauge(int x, int y, int r, int percent, uint16_t color, String label);
    
    void drawGearIcon(int x, int y, uint16_t color);
    void drawBellIcon(int x, int y, uint16_t color, int count);
    void drawLightIcon(int x, int y, uint16_t color, bool on);
    void drawAmsIcon(int x, int y, uint16_t color);
    void drawPrintIcon(int x, int y, uint16_t color);
};