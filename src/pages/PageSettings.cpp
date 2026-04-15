#include "PageSettings.h"
#include "PageManager.h"
#include <algorithm> // For std::min / std::max

#include "Version.h"

#include "../BambuMQTT.h"
extern BambuMQTT printerMqtt;

#ifndef STATE_OTA_UPDATE
#define STATE_OTA_UPDATE 13
#endif

PageSettings::PageSettings(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    btnBrightDown = {180, 80,  50, 40, "-", _tft->color565(71, 85, 105)};
    btnBrightUp   = {320, 80,  50, 40, "+", _tft->color565(71, 85, 105)};
    
    btnTimeDown   = {180, 140, 50, 40, "-", _tft->color565(71, 85, 105)};
    btnTimeUp     = {320, 140, 50, 40, "+", _tft->color565(71, 85, 105)};
    
    btnDumpMqtt   = {380, 140, 85, 40, "Dump MQTT", _tft->color565(71, 85, 105)};
    
    btnSwitchPrinter = {15,  210, 105, 40, "Printer", _tft->color565(37, 99, 235)};
    btnSelectWifi    = {130, 210, 105, 40, "WiFi",    _tft->color565(147, 51, 234)};
    btnLogout        = {245, 210, 105, 40, "Logout",  _tft->color565(245, 158, 11)};
    btnOTAUpdate     = {360, 210, 105, 40, "OTA Update", _tft->color565(20, 83, 45)};

    btnBack          = {15,  270, 140, 40, "<- Back", _tft->color565(20, 83, 45)};
    btnSetIP         = {165, 270, 145, 40, "Set IP",  _tft->color565(71, 85, 105)};
    btnSetCode       = {320, 270, 145, 40, "Set Code",_tft->color565(71, 85, 105)};
    btnWipe          = {415, 5, 50, 40, "WIPE", _tft->color565(220, 38, 38)}; // Moved to top right
    
    btnConfirmWipe   = {110, 210, 110, 50, "YES", _tft->color565(220, 38, 38)};
    btnCancelWipe    = {260, 210, 110, 50, "NO", _tft->color565(71, 85, 105)};
    
    _showModal = false;
}

void PageSettings::onEnter() {
    _showModal = false;
    _tft->fillScreen(_tft->color565(15, 23, 42)); // #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(30, 41, 59)); // Header #1e293b
    
    _tft->setTextDatum(MC_DATUM); // Force Center Alignment (Fixes print monitor rendering overlap)
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->drawString("Device Settings (v" + String(APP_VERSION) + ")", SCREEN_WIDTH/2, 25, 4);
    
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
    _tft->drawString("Brightness", 90, 100, 4);
    _tft->drawString("Timeout", 90, 160, 4);
    
    btnSetIP.label = config.activePrinter.ip == "" ? "Set Printer IP" : "IP: " + config.activePrinter.ip;
    
    String maskedCode = "";
    for (int i = 0; i < config.activePrinter.accessCode.length(); i++) maskedCode += "*";
    btnSetCode.label = config.activePrinter.accessCode == "" ? "Set Access Code" : "Code: " + maskedCode;

    Button btns[] = {btnBrightDown, btnBrightUp, btnTimeDown, btnTimeUp, btnSwitchPrinter, btnSelectWifi, btnLogout, btnOTAUpdate, btnWipe, btnBack, btnSetIP, btnSetCode, btnDumpMqtt};
    for (Button b : btns) {
        _tft->fillRect(b.x, b.y, b.w, b.h, b.color);
        _tft->drawRect(b.x, b.y, b.w, b.h, TFT_WHITE);
        _tft->setTextColor(TFT_WHITE, b.color); // FIXES BLACK BOXES IN BUTTONS!
        
        int font = (b.label.length() > 8) ? 2 : 4;
        if (b.label == "WIPE" || b.label == "Dump MQTT" || b.label.startsWith("IP:") || b.label.startsWith("Code:") || b.label.startsWith("Set ") || b.label == "OTA Update") font = 2;
        
        _tft->drawString(b.label, b.x + b.w/2, b.y + b.h/2, font);
    }
    
    drawValues();
}

void PageSettings::drawModal() {
    _tft->fillRoundRect(80, 90, 320, 200, 12, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(80, 90, 320, 200, 12, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->drawString("Factory Reset?", 240, 130, 4);
    _tft->drawString("This deletes all config.", 240, 170, 2);
    
    Button btns[] = {btnConfirmWipe, btnCancelWipe};
    for (Button b : btns) {
        _tft->fillRect(b.x, b.y, b.w, b.h, b.color);
        _tft->drawRect(b.x, b.y, b.w, b.h, TFT_WHITE);
        _tft->setTextColor(TFT_WHITE, b.color);
        _tft->drawString(b.label, b.x + b.w/2, b.y + b.h/2, 4);
    }
}

void PageSettings::drawValues() {
    _tft->setTextDatum(MC_DATUM); // Prevent alignment bleeding
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
    _tft->fillRect(240, 80, 70, 40, _tft->color565(15, 23, 42));
    int pct = round(config.brightness / 2.55);
    _tft->drawString(String(pct) + "%", 275, 100, 4);

    _tft->fillRect(240, 140, 70, 40, _tft->color565(15, 23, 42));
    String tStr = config.screenTimeout == 0 ? "Never" : String(config.screenTimeout) + "m";
    _tft->drawString(tStr, 275, 160, 4);
}

void PageSettings::onUpdate() {
    int x, y;
    if (_manager->readTouch(x, y)) {
        if (_showModal) {
            if (btnConfirmWipe.isTouched(x, y)) {
                factoryReset();
            } else if (btnCancelWipe.isTouched(x, y)) {
                onEnter(); // Redraw normal UI
            }
            return; // Stop processing background touches
        }
        
        int pct = round(config.brightness / 2.55);
        if (btnBack.isTouched(x, y)) {
            saveSettings(); _manager->switchPage(STATE_MAIN_PAGE);
        } else if (btnSetIP.isTouched(x, y)) {
            _manager->openKeyboard(&config.activePrinter.ip);
        } else if (btnSetCode.isTouched(x, y)) {
            _manager->openKeyboard(&config.activePrinter.accessCode);
        } else if (btnBrightUp.isTouched(x, y)) {
            pct = std::min(100, pct + 10); config.brightness = pct * 2.55; drawValues(); delay(150);
        } else if (btnBrightDown.isTouched(x, y)) {
            pct = std::max(10, pct - 10); config.brightness = pct * 2.55; drawValues(); delay(150);
        } else if (btnTimeUp.isTouched(x, y)) {
            config.screenTimeout += 1; drawValues(); delay(150);
        } else if (btnTimeDown.isTouched(x, y)) {
            config.screenTimeout = std::max(0, config.screenTimeout - 1); drawValues(); delay(150);
        } else if (btnDumpMqtt.isTouched(x, y)) {
            extern bool dumpNextMqttPacket;
            dumpNextMqttPacket = true;
            printerMqtt.requestStatusUpdate(); // Force the printer to send the massive "pushall" payload
            _tft->fillRect(btnDumpMqtt.x, btnDumpMqtt.y, btnDumpMqtt.w, btnDumpMqtt.h, TFT_GREEN);
            _tft->drawRect(btnDumpMqtt.x, btnDumpMqtt.y, btnDumpMqtt.w, btnDumpMqtt.h, TFT_WHITE);
            _tft->setTextColor(TFT_BLACK, TFT_GREEN);
            _tft->drawString("Waiting...", btnDumpMqtt.x + btnDumpMqtt.w/2, btnDumpMqtt.y + btnDumpMqtt.h/2, 2);
            delay(500); 
            onEnter(); // Reset visual state
        } else if (btnSwitchPrinter.isTouched(x, y)) {
            _manager->switchPage(STATE_PRINTER_SELECT);
        } else if (btnSelectWifi.isTouched(x, y)) {
            config.wifiSSID = ""; config.wifiPass = ""; saveSettings();
            _manager->switchPage(STATE_WIFI_SCAN);
        } else if (btnLogout.isTouched(x, y)) {
            config.cloudToken = ""; config.userEmail = ""; config.userPassword = ""; config.userId = ""; saveSettings(); _manager->switchPage(STATE_LOGIN);
        } else if (btnOTAUpdate.isTouched(x, y)) {
            _manager->switchPage(static_cast<AppState>(STATE_OTA_UPDATE));
        } else if (btnWipe.isTouched(x, y)) {
            _showModal = true; drawModal();
        }
    }
}