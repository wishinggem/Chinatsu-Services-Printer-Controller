#include "PageMain.h"
#include "PageManager.h"
#include "../BambuMQTT.h"

extern BambuMQTT printerMqtt;

PageMain::PageMain(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    // Define 4 large slots for the AMS display
    int startX = 24;
    int yPos = 65;
    int width = 96;
    int height = 180;
    int spacing = 16;
    
    for(int i=0; i<4; i++) {
        slots[i] = {startX + i*(width+spacing), yPos, width, height, "Slot " + String(i+1), TFT_WHITE};
    }
    
    lastUpdate = 0;
}

uint16_t PageMain::hexToRGB565(String hex) {
    if (hex.length() < 6) return TFT_WHITE; // Fallback
    long rgb = strtol(hex.substring(0,6).c_str(), NULL, 16);
    return _tft->color565((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

void PageMain::onEnter() {
    _tft->fillScreen(_tft->color565(15, 23, 42)); // Background #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(15, 118, 110)); // Header #0f766e
    _tft->setTextColor(TFT_WHITE);
    _tft->setTextDatum(ML_DATUM);
    _tft->drawString(config.activePrinter.name + " - AMS", 20, 25, 4);
    
    _tft->fillRoundRect(420, 10, 40, 30, 4, _tft->color565(255, 255, 255)); // Gear/Settings Button
    _tft->setTextColor(TFT_BLACK);
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("...", 440, 25, 2); // Visual indicator for settings
    
    _tft->setTextDatum(MC_DATUM);

    for(int i=0; i<4; i++) {
        lastAms[i].type = "";
        lastAms[i].color = "";
    }
    lastHumidity = "";
    drawLiveData();
}

void PageMain::drawLiveData() {
    for(int i=0; i<4; i++) {
        if (lastAms[i].type != config.liveData.ams[i].type || lastAms[i].color != config.liveData.ams[i].color) {
            lastAms[i].type = config.liveData.ams[i].type;
            lastAms[i].color = config.liveData.ams[i].color;
            
            // Clear the entire slot background + flanges area
            _tft->fillRect(slots[i].x - 8, slots[i].y - 6, slots[i].w + 16, slots[i].h + 12, _tft->color565(15, 23, 42));

            // Empty slot check
            if (lastAms[i].type == "" || lastAms[i].color == "") {
                _tft->fillRoundRect(slots[i].x, slots[i].y, slots[i].w, slots[i].h, 12, _tft->color565(30, 41, 59));
                for (int d = 0; d < slots[i].w + slots[i].h; d += 15) {
                    _tft->drawLine(slots[i].x + std::max(0, d - slots[i].h), slots[i].y + std::min(d, slots[i].h),
                                   slots[i].x + std::min(d, slots[i].w), slots[i].y + std::max(0, d - slots[i].w), 
                                   _tft->color565(51, 65, 85));
                }
                _tft->drawRoundRect(slots[i].x, slots[i].y, slots[i].w, slots[i].h, 12, _tft->color565(71, 85, 105));
                _tft->setTextColor(_tft->color565(148, 163, 184), _tft->color565(30, 41, 59));
                _tft->drawString("EMPTY", slots[i].x + slots[i].w/2, slots[i].y + slots[i].h/2, 4);
                continue;
            }
            
            uint16_t color = hexToRGB565(config.liveData.ams[i].color);
            
            // Draw Spool Flanges
            uint16_t flangeColor = _tft->color565(51, 65, 85);
            _tft->fillRoundRect(slots[i].x - 6, slots[i].y - 4, 12, slots[i].h + 8, 6, flangeColor);
            _tft->fillRoundRect(slots[i].x + slots[i].w - 6, slots[i].y - 4, 12, slots[i].h + 8, 6, flangeColor);
            
            // Draw Spool Base & Center
            _tft->fillRoundRect(slots[i].x, slots[i].y, slots[i].w, slots[i].h, 12, _tft->color565(20, 20, 20));
            _tft->fillRect(slots[i].x + 8, slots[i].y + 6, slots[i].w - 16, slots[i].h - 12, color);
            
            // Contrast text color based on background luminance (heuristic)
            long rgb = strtol(config.liveData.ams[i].color.substring(0,6).c_str(), NULL, 16);
            int r = (rgb >> 16) & 0xFF; int g = (rgb >> 8) & 0xFF; int b = rgb & 0xFF;
            float luminance = (0.299*r + 0.587*g + 0.114*b);
            _tft->setTextColor(luminance > 128 ? TFT_BLACK : TFT_WHITE, color);
            
            _tft->setTextDatum(MC_DATUM);
            
            // Multi-line text wrapping by spaces
            String typeText = config.liveData.ams[i].type;
            int yCursor = slots[i].y + slots[i].h / 2 - 20; 
            if (typeText.indexOf(' ') != -1) yCursor -= 15; 
            
            int start = 0;
            int lineCount = 0;
            while(start < typeText.length() && lineCount < 4) {
                int end = typeText.indexOf(' ', start);
                if (end == -1) end = typeText.length();
                String word = typeText.substring(start, end);
                if (word.length() > 0) {
                    _tft->drawString(word, slots[i].x + slots[i].w/2, yCursor, 4);
                    yCursor += 25;
                    lineCount++;
                }
                start = end + 1;
            }
        }
    }

    if (lastHumidity != config.liveData.amsHumidity) {
        lastHumidity = config.liveData.amsHumidity;
        
        _tft->fillRect(0, 260, 480, 60, _tft->color565(15, 23, 42)); // Clear footer
        _tft->drawFastHLine(0, 260, 480, _tft->color565(51, 65, 85)); // Top border
        _tft->setTextColor(_tft->color565(148, 163, 184), _tft->color565(15, 23, 42));
        
        String humPercent = "0%";
        if (lastHumidity == "1") humPercent = "10%";
        else if (lastHumidity == "2") humPercent = "20%";
        else if (lastHumidity == "3") humPercent = "30%";
        else if (lastHumidity == "4") humPercent = "40%";
        else if (lastHumidity == "5") humPercent = "50%";
        else humPercent = lastHumidity + "%";
        
        String icon = "AMS Humidity: " + humPercent;
        _tft->drawString(icon, SCREEN_WIDTH/2, 290, 4);
    }
}

void PageMain::onUpdate() {
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        drawLiveData();
    }

    int x, y;
    if (_manager->readTouch(x, y)) {
        if (x > 400 && y < 60) { // Top right corner (Settings)
            _manager->switchPage(STATE_SETTINGS);
        }
    }
}