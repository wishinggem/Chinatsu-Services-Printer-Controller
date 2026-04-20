#include "PageHomeAssistant.h"
#include "PageManager.h"
#include "../HomeAssistant.h"

extern HomeAssistant homeAssistant;

PageHomeAssistant::PageHomeAssistant(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    btnBack = {15, 270, 140, 40, "<- Back", _tft->color565(20, 83, 45)};
    btnDump = {170, 270, 140, 40, "Dump JSON", _tft->color565(51, 65, 85)};
    
    btnToggleSend = {15, 50, 210, 40, "", TFT_DARKGREY};
    btnToggleRecv = {240, 50, 210, 40, "", TFT_DARKGREY};
    
    inputIp       = {15, 105, 210, 40, "HA IP/Domain", &config.haIp};
    inputPort     = {240, 105, 210, 40, "HA Port", &config.tempHaPort};
    inputEndpoint = {15, 160, 435, 40, "HA Endpoint", &config.haEndpoint};
    inputInterval = {15, 215, 210, 40, "Send Interval (m)", &config.tempHaInterval};
}

void PageHomeAssistant::onEnter() {
    // Refresh string proxies from real integers
    config.tempHaPort = String(config.haPort);
    config.tempHaInterval = String(config.haSendInterval);

    _tft->fillScreen(_tft->color565(15, 23, 42));
    _tft->fillRect(0, 0, 480, 40, _tft->color565(30, 41, 59));
    
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Home Assistant Config", SCREEN_WIDTH/2, 20, 4);
    
    drawUI();
    
    _tft->fillRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, btnBack.color);
    _tft->drawRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, btnBack.color);
    _tft->drawString(btnBack.label, btnBack.x + btnBack.w/2, btnBack.y + btnBack.h/2, 4);
    
    _tft->fillRect(btnDump.x, btnDump.y, btnDump.w, btnDump.h, btnDump.color);
    _tft->drawRect(btnDump.x, btnDump.y, btnDump.w, btnDump.h, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, btnDump.color);
    _tft->drawString(btnDump.label, btnDump.x + btnDump.w/2, btnDump.y + btnDump.h/2, 2);
}

void PageHomeAssistant::drawUI() {
    btnToggleSend.color = config.haSendEnabled ? _tft->color565(5, 150, 105) : _tft->color565(220, 38, 38);
    btnToggleSend.label = config.haSendEnabled ? "Sending: ON" : "Sending: OFF";
    _tft->fillRect(btnToggleSend.x, btnToggleSend.y, btnToggleSend.w, btnToggleSend.h, btnToggleSend.color);
    _tft->drawRect(btnToggleSend.x, btnToggleSend.y, btnToggleSend.w, btnToggleSend.h, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, btnToggleSend.color);
    _tft->drawString(btnToggleSend.label, btnToggleSend.x + btnToggleSend.w/2, btnToggleSend.y + btnToggleSend.h/2, 2);

    btnToggleRecv.color = config.haReceiveEnabled ? _tft->color565(5, 150, 105) : _tft->color565(220, 38, 38);
    btnToggleRecv.label = config.haReceiveEnabled ? "Receive: ON" : "Receive: OFF";
    _tft->fillRect(btnToggleRecv.x, btnToggleRecv.y, btnToggleRecv.w, btnToggleRecv.h, btnToggleRecv.color);
    _tft->drawRect(btnToggleRecv.x, btnToggleRecv.y, btnToggleRecv.w, btnToggleRecv.h, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, btnToggleRecv.color);
    _tft->drawString(btnToggleRecv.label, btnToggleRecv.x + btnToggleRecv.w/2, btnToggleRecv.y + btnToggleRecv.h/2, 2);

    TextInput inputs[] = {inputIp, inputPort, inputEndpoint, inputInterval};
    for (TextInput input : inputs) {
        _tft->fillRect(input.x, input.y, input.w, input.h, _tft->color565(51, 65, 85));
        _tft->drawRect(input.x, input.y, input.w, input.h, TFT_WHITE);
        _tft->setTextColor(TFT_WHITE, _tft->color565(51, 65, 85));
        _tft->setTextDatum(ML_DATUM);
        _tft->drawString(" " + input.label + ":", input.x, input.y + 10, 2);
        
        String val = *input.valuePtr;
        if (val == "") val = "Tap to set";
        
        // Truncate for display if needed
        if (val.length() > 22 && input.label != "HA Endpoint") val = val.substring(0, 19) + "...";
        else if (val.length() > 40) val = val.substring(0, 37) + "...";
        
        _tft->drawString(" " + val, input.x, input.y + 26, 2);
    }
    _tft->setTextDatum(MC_DATUM);
}

void PageHomeAssistant::onUpdate() {
    int x, y;
    if (_manager->readTouch(x, y)) {
        if (btnBack.isTouched(x, y)) {
            saveSettings();
            _manager->switchPage(STATE_SETTINGS);
        }
        else if (btnDump.isTouched(x, y)) {
            homeAssistant.dumpPayload();
            _tft->fillRect(btnDump.x, btnDump.y, btnDump.w, btnDump.h, TFT_DARKGREEN);
            _tft->drawRect(btnDump.x, btnDump.y, btnDump.w, btnDump.h, TFT_WHITE);
            _tft->setTextColor(TFT_WHITE, TFT_DARKGREEN);
            _tft->drawString("Dumped!", btnDump.x + btnDump.w/2, btnDump.y + btnDump.h/2, 2);
            delay(200);
            _tft->fillRect(btnDump.x, btnDump.y, btnDump.w, btnDump.h, btnDump.color);
            _tft->drawRect(btnDump.x, btnDump.y, btnDump.w, btnDump.h, TFT_WHITE);
            _tft->setTextColor(TFT_WHITE, btnDump.color);
            _tft->drawString(btnDump.label, btnDump.x + btnDump.w/2, btnDump.y + btnDump.h/2, 2);
        }
        else if (btnToggleSend.isTouched(x, y)) {
            config.haSendEnabled = !config.haSendEnabled;
            drawUI();
        }
        else if (btnToggleRecv.isTouched(x, y)) {
            config.haReceiveEnabled = !config.haReceiveEnabled;
            drawUI();
        }
        else if (inputIp.isTouched(x, y)) {
            _manager->openKeyboard(&config.haIp);
        }
        else if (inputPort.isTouched(x, y)) {
            _manager->openKeyboard(&config.tempHaPort);
        }
        else if (inputEndpoint.isTouched(x, y)) {
            _manager->openKeyboard(&config.haEndpoint);
        }
        else if (inputInterval.isTouched(x, y)) {
            _manager->openKeyboard(&config.tempHaInterval);
        }
    }
}