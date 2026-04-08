#include "PageSettings.h"
#include "PageManager.h"
#include <algorithm> // For std::min / std::max

PageSettings::PageSettings(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    btnBrightDown = {180, 80,  50, 40, "-", _tft->color565(71, 85, 105)};
    btnBrightUp   = {320, 80,  50, 40, "+", _tft->color565(71, 85, 105)};
    
    btnTimeDown   = {180, 140, 50, 40, "-", _tft->color565(71, 85, 105)};
    btnTimeUp     = {320, 140, 50, 40, "+", _tft->color565(71, 85, 105)};

    // Shortened labels for Font 4 crispness!
    btnSwitchPrinter = {15,  210, 140, 40, "Printer", _tft->color565(37, 99, 235)};
    btnSelectWifi    = {165, 210, 140, 40, "WiFi",    _tft->color565(147, 51, 234)};
    btnLogout        = {315, 210, 150, 40, "Logout",  _tft->color565(245, 158, 11)};
    
    btnBack          = {120, 270, 240, 40, "<- Back", _tft->color565(20, 83, 45)};
    btnWipe          = {415, 5, 50, 40, "WIPE", _tft->color565(220, 38, 38)}; // Moved to top right
    
    btnConfirmWipe   = {110, 210, 110, 50, "YES", _tft->color565(220, 38, 38)};
    btnCancelWipe    = {260, 210, 110, 50, "NO", _tft->color565(71, 85, 105)};
    
    _showModal = false;
}

void PageSettings::onEnter() {
    _showModal = false;
    _tft->fillScreen(_tft->color565(15, 23, 42)); // #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(30, 41, 59)); // Header #1e293b
    
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->drawString("Device Settings", SCREEN_WIDTH/2, 25, 4);
    
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
    _tft->drawString("Brightness", 90, 100, 4);
    _tft->drawString("Timeout", 90, 160, 4);
    
    Button btns[] = {btnBrightDown, btnBrightUp, btnTimeDown, btnTimeUp, btnSwitchPrinter, btnSelectWifi, btnLogout, btnWipe, btnBack};
    for (Button b : btns) {
        _tft->fillRect(b.x, b.y, b.w, b.h, b.color);
        _tft->drawRect(b.x, b.y, b.w, b.h, TFT_WHITE);
        _tft->setTextColor(TFT_WHITE, b.color); // FIXES BLACK BOXES IN BUTTONS!
        int font = (b.label == "WIPE") ? 2 : 4;
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
        } else if (btnBrightUp.isTouched(x, y)) {
            pct = std::min(100, pct + 10); config.brightness = pct * 2.55; drawValues(); delay(150);
        } else if (btnBrightDown.isTouched(x, y)) {
            pct = std::max(10, pct - 10); config.brightness = pct * 2.55; drawValues(); delay(150);
        } else if (btnTimeUp.isTouched(x, y)) {
            config.screenTimeout += 1; drawValues(); delay(150);
        } else if (btnTimeDown.isTouched(x, y)) {
            config.screenTimeout = std::max(0, config.screenTimeout - 1); drawValues(); delay(150);
        } else if (btnSwitchPrinter.isTouched(x, y)) {
            _manager->switchPage(STATE_PRINTER_SELECT);
        } else if (btnSelectWifi.isTouched(x, y)) {
            config.wifiSSID = ""; config.wifiPass = ""; saveSettings();
            _manager->switchPage(STATE_WIFI_SCAN);
        } else if (btnLogout.isTouched(x, y)) {
            config.cloudToken = ""; config.userEmail = ""; config.userPassword = ""; config.userId = ""; saveSettings(); _manager->switchPage(STATE_LOGIN);
        } else if (btnWipe.isTouched(x, y)) {
            _showModal = true; drawModal();
        }
    }
}