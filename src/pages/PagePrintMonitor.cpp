#include "PagePrintMonitor.h"
#include "PageManager.h"
#include "../BambuMQTT.h"

#ifndef STATE_NOTIFICATIONS
#define STATE_NOTIFICATIONS 10
#endif

#ifndef STATE_MAIN_PAGE
#define STATE_MAIN_PAGE 3
#endif

extern BambuMQTT printerMqtt;
extern bool chamberLightOn;

extern int printLayer;
extern int printTotalLayers;
extern int printRemainingTime;
extern int printSpeedLevel;
extern int fanPartSpeed;
extern int fanAuxSpeed;
extern int fanChamberSpeed;

PagePrintMonitor::PagePrintMonitor(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    lastUpdate = 0;
}

void PagePrintMonitor::drawGearIcon(int x, int y, uint16_t color) {
    _tft->fillCircle(x, y, 11, color);
    _tft->fillCircle(x, y, 5, _tft->color565(30, 41, 59));
    for (int i=0; i<8; i++) {
        float angle = i * 45.0 * 3.14159 / 180.0;
        _tft->fillCircle(x + cos(angle) * 14, y + sin(angle) * 14, 3, color);
    }
}

void PagePrintMonitor::drawBellIcon(int x, int y, uint16_t color, int count) {
    y -= 2; // Mathematically shifts icon up to perfectly center it within the header
    _tft->fillCircle(x, y - 2, 5, color);
    _tft->fillTriangle(x - 8, y + 6, x + 8, y + 6, x, y - 5, color);
    _tft->fillRect(x - 8, y + 6, 17, 2, color);
    _tft->fillCircle(x, y + 9, 3, color);
    if (count > 0) {
        _tft->fillCircle(x + 10, y - 8, 8, TFT_RED);
        _tft->setTextColor(TFT_WHITE, TFT_RED);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString(String(count), x + 10, y - 8, 1);
    }
}

void PagePrintMonitor::drawLightIcon(int x, int y, uint16_t color, bool on) {
    uint16_t bulbColor = on ? _tft->color565(250, 204, 21) : color;
    _tft->fillRect(x - 3, y + 4, 7, 5, _tft->color565(148, 163, 184));
    _tft->fillRect(x - 2, y + 9, 5, 2, _tft->color565(71, 85, 105));
    _tft->fillCircle(x, y - 2, 6, bulbColor);
    _tft->fillRect(x - 4, y + 1, 9, 4, bulbColor);
    if (on) {
        _tft->drawFastVLine(x, y - 13, 3, bulbColor);
        _tft->drawFastHLine(x - 12, y - 2, 3, bulbColor);
        _tft->drawFastHLine(x + 10, y - 2, 3, bulbColor);
        _tft->drawPixel(x - 8, y - 9, bulbColor);
        _tft->drawPixel(x - 7, y - 8, bulbColor);
        _tft->drawPixel(x + 8, y - 9, bulbColor);
        _tft->drawPixel(x + 7, y - 8, bulbColor);
    }
}

void PagePrintMonitor::drawArcGauge(int x, int y, int r, int percent, uint16_t color, String label) {
    // Clear drawing area
    _tft->fillRect(x - r - 2, y - r - 2, (r * 2) + 4, r + 34, _tft->color565(15, 23, 42));
    
    // Draw background arc (Slate 600)
    _tft->drawSmoothArc(x, y, r, r - 12, 90, 270, _tft->color565(71, 85, 105), _tft->color565(15, 23, 42));
    
    // Draw foreground filled arc
    if (percent > 0) {
        int endAngle = 90 + (180 * percent / 100);
        if (endAngle > 270) endAngle = 270;
        _tft->drawSmoothArc(x, y, r, r - 12, 90, endAngle, color, _tft->color565(15, 23, 42));
    }
    
    _tft->setTextColor(_tft->color565(148, 163, 184), _tft->color565(15, 23, 42));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(label, x, y + 18, 2);
    
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
    _tft->drawString(String(percent) + "%", x, y - 5, 4); 
}

void PagePrintMonitor::drawAmsIcon(int x, int y, uint16_t color) {
    _tft->drawRoundRect(x - 14, y - 8, 29, 16, 3, color);
    _tft->drawCircle(x - 7, y, 4, color);
    _tft->drawCircle(x + 7, y, 4, color);
}

void PagePrintMonitor::drawPrintIcon(int x, int y, uint16_t color) {
    _tft->fillRect(x - 6, y - 8, 13, 10, color);
    _tft->fillTriangle(x - 6, y + 2, x + 6, y + 2, x, y + 8, color);
    _tft->drawFastHLine(x - 4, y - 10, 9, color);
}

void PagePrintMonitor::onEnter() {
    _tft->fillScreen(_tft->color565(15, 23, 42)); 
    _tft->fillRect(0, 0, 480, 50, _tft->color565(15, 118, 110)); 
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 118, 110));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(config.activePrinter.name, 240, 25, 4);
    
    _tft->fillRoundRect(15, 5, 50, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(15, 5, 50, 40, 8, TFT_WHITE);
    lastLightState = chamberLightOn;
    drawLightIcon(40, 25, TFT_WHITE, chamberLightOn);

    _tft->fillRoundRect(355, 5, 50, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(355, 5, 50, 40, 8, TFT_WHITE);
    drawBellIcon(380, 25, TFT_WHITE, hmsHistory.size());

    _tft->fillRoundRect(415, 5, 50, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(415, 5, 50, 40, 8, TFT_WHITE);
    drawGearIcon(440, 25, TFT_WHITE);
    
    _tft->drawFastHLine(0, 260, 480, _tft->color565(51, 65, 85)); // Footer border
    
    // Draw Bottom Navigation Buttons
    _tft->fillRoundRect(10, 270, 90, 40, 8, _tft->color565(30, 41, 59)); // AMS is inactive
    _tft->drawRoundRect(10, 270, 90, 40, 8, TFT_WHITE);
    drawAmsIcon(32, 290, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("AMS", 68, 290, 2);
    
    _tft->fillRoundRect(110, 270, 90, 40, 8, _tft->color565(56, 189, 248)); // Print is active
    _tft->drawRoundRect(110, 270, 90, 40, 8, TFT_WHITE);
    drawPrintIcon(132, 290, TFT_BLACK);
    _tft->setTextColor(TFT_BLACK, _tft->color565(56, 189, 248));
    _tft->drawString("PRINT", 168, 290, 2);

    lastStatus = "FORCE";
    notificationDrawn = false;
    drawLiveData();
}

void PagePrintMonitor::drawLiveData() {
    if (lastLightState != chamberLightOn) {
        lastLightState = chamberLightOn;
        _tft->fillRoundRect(15, 5, 50, 40, 8, _tft->color565(30, 41, 59));
        _tft->drawRoundRect(15, 5, 50, 40, 8, TFT_WHITE);
        drawLightIcon(40, 25, TFT_WHITE, chamberLightOn);
    }

    if (lastStatus != config.liveData.status) {
        lastStatus = config.liveData.status;
        _tft->fillRect(0, 50, 480, 210, _tft->color565(15, 23, 42)); // Clear working area
        
        // Force all values to redraw
        lastLayer = -2; lastTotalLayers = -2; lastTime = -2; lastSpeed = -2;
        lastPart = -1; lastAux = -1; lastChamber = -1;
        lastNoz = -1; lastNozTarget = -1; lastBed = -1; lastBedTarget = -1;
    }

    bool isPrinting = !(config.liveData.status == "Idle" || config.liveData.status == "Offline" || config.liveData.status == "");

    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
    
    // LEFT COLUMN: Layer, Time, Speed
    if (lastLayer != printLayer || lastTotalLayers != printTotalLayers) {
        lastLayer = printLayer; lastTotalLayers = printTotalLayers;
        _tft->fillRect(10, 60, 140, 50, _tft->color565(15, 23, 42));
        _tft->setTextDatum(TL_DATUM);
        _tft->drawString("Layer", 15, 65, 2);
        if (isPrinting) {
            _tft->drawString(String(printLayer) + " / " + String(printTotalLayers), 15, 85, 4);
        } else {
            _tft->drawString("No print", 15, 85, 4);
        }
    }
    
    if (lastTime != printRemainingTime) {
        lastTime = printRemainingTime;
        _tft->fillRect(10, 120, 140, 50, _tft->color565(15, 23, 42));
        _tft->setTextDatum(TL_DATUM);
        _tft->drawString("Remaining", 15, 125, 2);
        if (isPrinting) {
            int h = printRemainingTime / 60;
            int m = printRemainingTime % 60;
            _tft->drawString(String(h) + "h " + String(m) + "m", 15, 145, 4);
        } else {
            _tft->drawString("No print", 15, 145, 4);
        }
    }
    
    if (lastSpeed != printSpeedLevel) {
        lastSpeed = printSpeedLevel;
        String speedTxt = "Standard";
        if (printSpeedLevel == 1) speedTxt = "Silent";
        else if (printSpeedLevel == 3) speedTxt = "Sport";
        else if (printSpeedLevel == 4) speedTxt = "Ludicrous";
        _tft->fillRect(10, 180, 140, 50, _tft->color565(15, 23, 42));
        _tft->setTextDatum(TL_DATUM);
        _tft->drawString("Speed", 15, 185, 2);
        _tft->drawString(speedTxt, 15, 205, 4);
    }

    // RIGHT COLUMN: Temps, Fans
    if (lastNoz != config.liveData.nozzleTemp || lastNozTarget != config.liveData.nozzleTarget) {
        lastNoz = config.liveData.nozzleTemp; lastNozTarget = config.liveData.nozzleTarget;
        _tft->fillRect(160, 60, 150, 50, _tft->color565(15, 23, 42));
        _tft->setTextDatum(TL_DATUM);
        _tft->setTextColor(_tft->color565(148, 163, 184), _tft->color565(15, 23, 42));
        _tft->drawString("Nozzle", 165, 65, 2);
        _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
        _tft->drawString(String((int)config.liveData.nozzleTemp) + " / " + String((int)config.liveData.nozzleTarget) + " C", 165, 85, 4);
    }

    if (lastBed != config.liveData.bedTemp || lastBedTarget != config.liveData.bedTarget) {
        lastBed = config.liveData.bedTemp; lastBedTarget = config.liveData.bedTarget;
        _tft->fillRect(320, 60, 150, 50, _tft->color565(15, 23, 42));
        _tft->setTextDatum(TL_DATUM);
        _tft->setTextColor(_tft->color565(148, 163, 184), _tft->color565(15, 23, 42));
        _tft->drawString("Bed", 325, 65, 2);
        _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
        _tft->drawString(String((int)config.liveData.bedTemp) + " / " + String((int)config.liveData.bedTarget) + " C", 325, 85, 4);
    }

    // Render the beautiful half-circle fan gauges
    if (lastPart != fanPartSpeed) { lastPart = fanPartSpeed; drawArcGauge(215, 200, 45, fanPartSpeed, _tft->color565(56, 189, 248), "Part"); }
    if (lastAux != fanAuxSpeed) { lastAux = fanAuxSpeed; drawArcGauge(320, 200, 45, fanAuxSpeed, TFT_WHITE, "Aux"); }
    if (lastChamber != fanChamberSpeed) { lastChamber = fanChamberSpeed; drawArcGauge(425, 200, 45, fanChamberSpeed, _tft->color565(148, 163, 184), "Chamber"); }
}

void PagePrintMonitor::onUpdate() {
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        if (!notificationDrawn) {
            drawLiveData();
        }
    }

    int x, y;
    if (_manager->readTouch(x, y)) {
        if (notificationDrawn && x >= 40 && x <= 440 && y >= 60 && y <= 100) {
            notificationDrawn = false;
            _manager->switchPage(static_cast<AppState>(STATE_NOTIFICATIONS));
            return;
        }
        if (x >= 15 && x <= 65 && y <= 60) { // Light Button
            chamberLightOn = !chamberLightOn;
            printerMqtt.setChamberLight(chamberLightOn);
            _tft->fillRoundRect(15, 5, 50, 40, 8, _tft->color565(30, 41, 59));
            _tft->drawRoundRect(15, 5, 50, 40, 8, TFT_WHITE);
            drawLightIcon(40, 25, TFT_WHITE, chamberLightOn);
            lastLightState = chamberLightOn;
            return;
        }
        if (x >= 10 && x <= 100 && y >= 270 && y <= 310) { // AMS Button
            _manager->switchPage(static_cast<AppState>(STATE_MAIN_PAGE));
            return;
        }
        if (x > 355 && x < 405 && y < 60) { // Notification Bell
            _manager->switchPage(static_cast<AppState>(STATE_NOTIFICATIONS));
            return;
        }
        if (x > 400 && y < 60) { // Top right corner (Settings)
            _manager->switchPage(STATE_SETTINGS);
        }
    }
}