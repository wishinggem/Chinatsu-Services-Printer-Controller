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

void PageMain::drawGearIcon(int x, int y, uint16_t color) {
    _tft->fillCircle(x, y, 11, color);
    _tft->fillCircle(x, y, 5, _tft->color565(30, 41, 59)); // Inner hole matches button background
    for (int i=0; i<8; i++) {
        float angle = i * 45.0 * 3.14159 / 180.0;
        _tft->fillCircle(x + cos(angle) * 14, y + sin(angle) * 14, 3, color);
    }
}

void PageMain::drawDropIcon(int x, int y, uint16_t color) {
    _tft->fillCircle(x, y + 2, 8, color);
    _tft->fillTriangle(x - 8, y + 2, x + 8, y + 2, x, y - 11, color);
}

void PageMain::onEnter() {
    _tft->fillScreen(_tft->color565(15, 23, 42)); // Background #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(15, 118, 110)); // Header #0f766e
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 118, 110));
    _tft->setTextDatum(ML_DATUM);
    _tft->drawString(config.activePrinter.name, 20, 25, 4);
    
    // Draw custom gear icon for Settings button
    _tft->fillRoundRect(415, 5, 50, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(415, 5, 50, 40, 8, TFT_WHITE);
    drawGearIcon(440, 25, TFT_WHITE);
    
    _tft->setTextDatum(MC_DATUM);

    for(int i=0; i<4; i++) {
        lastAms[i].type = "FORCE_UPDATE";
        lastAms[i].color = "FORCE_UPDATE";
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
                uint16_t flangeColor = _tft->color565(51, 65, 85);
                _tft->fillRoundRect(slots[i].x - 6, slots[i].y - 4, 12, slots[i].h + 8, 6, flangeColor);
                _tft->fillRoundRect(slots[i].x + slots[i].w - 6, slots[i].y - 4, 12, slots[i].h + 8, 6, flangeColor);
                
                _tft->fillRoundRect(slots[i].x, slots[i].y, slots[i].w, slots[i].h, 12, _tft->color565(20, 20, 20));
                _tft->fillRect(slots[i].x + 8, slots[i].y + 6, slots[i].w - 16, slots[i].h - 12, _tft->color565(30, 41, 59));
                
                for (int d = slots[i].y + 20; d < slots[i].y + slots[i].h - 10; d += 20) {
                    _tft->drawFastHLine(slots[i].x + 8, d, slots[i].w - 16, _tft->color565(51, 65, 85));
                }
                _tft->setTextColor(_tft->color565(148, 163, 184), _tft->color565(30, 41, 59));
                _tft->drawString("EMPTY", slots[i].x + slots[i].w/2, slots[i].y + slots[i].h/2, 2);
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
            typeText.replace("-", " - "); // Dynamically break on dashes too!
            
            std::vector<String> lines;
            int start = 0;
            while(start < typeText.length() && lines.size() < 4) {
                int end = typeText.indexOf(' ', start);
                if (end == -1) end = typeText.length();
                String word = typeText.substring(start, end);
                word.trim();
                if (word.length() > 0) {
                    lines.push_back(word);
                }
                start = end + 1;
            }
            
            int yCursor = slots[i].y + slots[i].h / 2 - ((lines.size() - 1) * 25) / 2;
            for (size_t j = 0; j < lines.size(); j++) {
                _tft->drawString(lines[j], slots[i].x + slots[i].w/2, yCursor, 4);
                yCursor += 25;
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
        
        _tft->setTextDatum(ML_DATUM);
        drawDropIcon(160, 288, _tft->color565(56, 189, 248)); // #38bdf8 light blue
        _tft->drawString("Humidity: " + humPercent, 175, 290, 4);
        _tft->setTextDatum(MC_DATUM);
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