#include "PageSettings.h"
#include "PageManager.h"
#include <algorithm> // For std::min / std::max

PageSettings::PageSettings(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    btnBrightDown = {180, 80,  50, 40, "-", _tft->color565(71, 85, 105)};
    btnBrightUp   = {320, 80,  50, 40, "+", _tft->color565(71, 85, 105)};
    
    btnTimeDown   = {180, 140, 50, 40, "-", _tft->color565(71, 85, 105)};
    btnTimeUp     = {320, 140, 50, 40, "+", _tft->color565(71, 85, 105)};

    btnSwitchPrinter = {15,  210, 140, 40, "Switch Printer", _tft->color565(37, 99, 235)};
    btnSelectWifi    = {165, 210, 140, 40, "Select WiFi",    _tft->color565(147, 51, 234)};
    btnLogout        = {315, 210, 150, 40, "Logout Cloud",   _tft->color565(245, 158, 11)};
    
    btnBack          = {0, 270, 240, 50, "<- Back to AMS", _tft->color565(15, 23, 42)};
    btnWipe          = {240, 270, 240, 50, "Factory Reset", _tft->color565(220, 38, 38)};
}

void PageSettings::onEnter() {
    _tft->fillScreen(_tft->color565(15, 23, 42)); // #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(30, 41, 59)); // Header #1e293b
    _tft->drawString("Device Settings", SCREEN_WIDTH/2, 25, 4);
    
    _tft->drawString("Brightness", 90, 100, 2);
    _tft->drawString("Timeout", 90, 160, 2);
    
    Button btns[] = {btnBrightDown, btnBrightUp, btnTimeDown, btnTimeUp, btnSwitchPrinter, btnSelectWifi, btnLogout, btnWipe, btnBack};
    for (Button b : btns) {
        _tft->fillRect(b.x, b.y, b.w, b.h, b.color);
        _tft->drawRect(b.x, b.y, b.w, b.h, TFT_WHITE);
        _tft->drawString(b.label, b.x + b.w/2, b.y + b.h/2, b.label == "-" || b.label == "+" ? 4 : 2);
    }
    
    drawValues();
}

void PageSettings::drawValues() {
    _tft->fillRect(240, 80, 70, 40, _tft->color565(15, 23, 42));
    _tft->drawString(String(map(config.brightness, 0, 255, 0, 100)) + "%", 275, 100, 4);

    _tft->fillRect(240, 140, 70, 40, _tft->color565(15, 23, 42));
    String tStr = config.screenTimeout == 0 ? "Never" : String(config.screenTimeout) + "m";
    _tft->drawString(tStr, 275, 160, 4);
}

void PageSettings::onUpdate() {
    int x, y;
    if (_manager->readTouch(x, y)) {
        if (btnBack.isTouched(x, y)) {
            saveSettings(); _manager->switchPage(STATE_MAIN_PAGE);
        } else if (btnBrightUp.isTouched(x, y)) {
            config.brightness = std::min(255, config.brightness + 25); drawValues(); delay(150);
        } else if (btnBrightDown.isTouched(x, y)) {
            config.brightness = std::max(10, config.brightness - 25); drawValues(); delay(150);
        } else if (btnTimeUp.isTouched(x, y)) {
            config.screenTimeout += 1; drawValues(); delay(150);
        } else if (btnTimeDown.isTouched(x, y)) {
            config.screenTimeout = std::max(0, config.screenTimeout - 1); drawValues(); delay(150);
        } else if (btnSwitchPrinter.isTouched(x, y)) {
            _manager->switchPage(STATE_PRINTER_SELECT);
        } else if (btnSelectWifi.isTouched(x, y)) {
            _manager->switchPage(STATE_WIFI_SCAN);
        } else if (btnLogout.isTouched(x, y)) {
            config.cloudToken = ""; config.userEmail = ""; config.userPassword = ""; config.userId = ""; saveSettings(); _manager->switchPage(STATE_LOGIN);
        } else if (btnWipe.isTouched(x, y)) {
            factoryReset();
        }
    }
}