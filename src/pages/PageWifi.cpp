#include "PageWifi.h"
#include "PageManager.h"
#include <WiFi.h>
#include "../BambuMQTT.h"

extern BambuMQTT printerMqtt;

PageWifi::PageWifi(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    btnSkip = {80, 250, 140, 50, "Skip", TFT_DARKGREY};
    btnRefresh = {260, 250, 140, 50, "Refresh", TFT_BLUE};
    btnReset = {380, 5, 95, 30, "RESET", TFT_MAROON}; // Added Reset
    
    _state = SCANNING;
    loadingDots = 0;
    lastDotTime = 0;
}

void PageWifi::onEnter() {
    _tft->drawString("WiFi Setup", SCREEN_WIDTH/2, 15, 4);
    
    // Draw Reset
    _tft->fillRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, btnReset.color);
    _tft->drawRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, TFT_WHITE);
    _tft->drawString(btnReset.label, btnReset.x + (btnReset.w/2), btnReset.y + (btnReset.h/2), 2);
    
    if (config.wifiPass.length() > 0 && config.wifiSSID.length() > 0) {
        _state = CONNECTING;
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.setAutoReconnect(true);
        WiFi.begin(config.wifiSSID.c_str(), config.wifiPass.c_str());
        _tft->fillRect(0, 80, SCREEN_WIDTH, 200, TFT_BLACK);
        _tft->drawString("Connecting to:", SCREEN_WIDTH/2, 120, 2);
        _tft->drawString(config.wifiSSID, SCREEN_WIDTH/2, 160, 4);
        connectStartTime = millis();
        return;
    }

    if (WiFi.scanComplete() >= 0) {
        _state = SELECTING;
        drawNetworks();
        return;
    }

    _state = SCANNING;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.scanNetworks(true); 
}

void PageWifi::drawNetworks() {
    _tft->fillRect(0, 80, SCREEN_WIDTH, 160, TFT_BLACK); 
    int n = WiFi.scanComplete();
    networkButtons.clear();
    
    int displayed = 0;
    for (int i = 0; i < n && displayed < 3; ++i) {
        String ssid = WiFi.SSID(i);
        
        // Skip hidden networks
        if (ssid.length() == 0) continue;
        
        // Skip duplicate network names (e.g., mesh nodes or dual-band routers)
        bool isDuplicate = false;
        for (const auto& btn : networkButtons) {
            if (btn.label == ssid) {
                isDuplicate = true;
                break;
            }
        }
        if (isDuplicate) continue;
        
        int yPos = 80 + (displayed * 55);
        networkButtons.push_back({40, yPos, 400, 45, ssid, TFT_DARKCYAN});
        
        _tft->fillRect(40, yPos, 400, 45, TFT_DARKCYAN);
        _tft->drawString(ssid, SCREEN_WIDTH/2, yPos + 22, 2);
        
        displayed++;
    }
    
    _tft->fillRect(btnSkip.x, btnSkip.y, btnSkip.w, btnSkip.h, btnSkip.color);
    _tft->drawString(btnSkip.label, btnSkip.x + (btnSkip.w/2), btnSkip.y + (btnSkip.h/2), 2);

    _tft->fillRect(btnRefresh.x, btnRefresh.y, btnRefresh.w, btnRefresh.h, btnRefresh.color);
    _tft->drawString(btnRefresh.label, btnRefresh.x + (btnRefresh.w/2), btnRefresh.y + (btnRefresh.h/2), 2);
}

void PageWifi::onUpdate() {
    int touchX, touchY;
    bool isTouched = _manager->readTouch(touchX, touchY);

    // EMERGENCY RESET CHECK
    if (isTouched && btnReset.isTouched(touchX, touchY)) {
        factoryReset(); 
        return;
    }

    if (_state == SCANNING) {
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            if (millis() - lastDotTime > 500) {
                lastDotTime = millis();
                loadingDots = (loadingDots + 1) % 4;
                String dots = "";
                for(int i=0; i<loadingDots; i++) dots += ".";
                
                _tft->fillRect(0, 35, SCREEN_WIDTH, 30, TFT_BLACK);
                _tft->drawString("Scanning in background" + dots, SCREEN_WIDTH/2, 50, 2);
            }
        } else if (n >= 0) {
            _state = SELECTING;
            _tft->fillRect(0, 35, SCREEN_WIDTH, 30, TFT_BLACK);
            _tft->drawString("Scan Complete!", SCREEN_WIDTH/2, 50, 2);
            
            if (!_manager->isKeyboardOpen()) {
                drawNetworks();
            }
        }
    }
    
    else if (_state == SELECTING) {
        if (isTouched) {
            if (btnSkip.isTouched(touchX, touchY)) {
                WiFi.scanDelete(); 
                _manager->switchPage(STATE_LOGIN);
            }
            else if (btnRefresh.isTouched(touchX, touchY)) {
                _tft->fillRect(0, 80, SCREEN_WIDTH, 240, TFT_BLACK); 
                WiFi.scanDelete();
                _state = SCANNING;
                WiFi.mode(WIFI_STA);
                WiFi.disconnect();
                delay(100);
                WiFi.scanNetworks(true);
            }
            
            for (auto& btn : networkButtons) {
                if (btn.isTouched(touchX, touchY)) {
                    config.wifiSSID = btn.label;
                    config.wifiPass = ""; 
                    _manager->openKeyboard(&config.wifiPass);
                }
            }
        }
    }
    
    else if (_state == CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _tft->fillRect(0, 200, SCREEN_WIDTH, 50, TFT_BLACK);
            _tft->setTextColor(TFT_GREEN);
            _tft->drawString("CONNECTED!", SCREEN_WIDTH/2, 220, 4);
            _tft->setTextColor(TFT_WHITE);
            saveSettings(); 
            delay(1000);
            
            if (config.cloudToken.length() > 0 && config.activePrinter.serial != "") {
                printerMqtt.begin(config.activePrinter.serial, config.userId, config.cloudToken);
                _manager->switchPage(STATE_MAIN_PAGE);
            } else {
                _manager->switchPage(STATE_LOGIN);
            }
        } else if (WiFi.status() == WL_CONNECT_FAILED || (millis() - connectStartTime > 15000)) { // 15-second visual timeout
            _tft->fillRect(0, 200, SCREEN_WIDTH, 50, TFT_BLACK);
            _tft->setTextColor(TFT_RED);
            _tft->drawString("Failed! Bad Password.", SCREEN_WIDTH/2, 220, 2);
            _tft->setTextColor(TFT_WHITE);
            delay(2000);
            config.wifiPass = "";
            WiFi.scanDelete(); 
            _tft->fillRect(0, 80, SCREEN_WIDTH, 240, TFT_BLACK); 
            _state = SCANNING;
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(100);
            WiFi.scanNetworks(true);
        }
    }
}