#include "PageMain.h"
#include "PageManager.h"
#include "../BambuMQTT.h"

#ifndef STATE_NOTIFICATIONS
#define STATE_NOTIFICATIONS 10
#endif

#ifndef STATE_PRINT_MONITOR
#define STATE_PRINT_MONITOR 11
#endif

extern BambuMQTT printerMqtt;
extern bool chamberLightOn;
extern int amsRemain[4];
extern String amsBrand[4];

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

void PageMain::drawBellIcon(int x, int y, uint16_t color, int count) {
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

void PageMain::drawLightIcon(int x, int y, uint16_t color, bool on) {
    uint16_t bulbColor = on ? _tft->color565(250, 204, 21) : color; // Yellowish when on, white/gray when off
    
    // Base of the bulb
    _tft->fillRect(x - 3, y + 4, 7, 5, _tft->color565(148, 163, 184)); // Slate 400
    _tft->fillRect(x - 2, y + 9, 5, 2, _tft->color565(71, 85, 105)); // Slate 600
    
    // Glass bulb
    _tft->fillCircle(x, y - 2, 6, bulbColor);
    _tft->fillRect(x - 4, y + 1, 9, 4, bulbColor);
    
    // Draw light rays when on
    if (on) {
        _tft->drawFastVLine(x, y - 13, 3, bulbColor);     // Top
        _tft->drawFastHLine(x - 12, y - 2, 3, bulbColor); // Left
        _tft->drawFastHLine(x + 10, y - 2, 3, bulbColor); // Right
        _tft->drawPixel(x - 8, y - 9, bulbColor);         // Top-left
        _tft->drawPixel(x - 7, y - 8, bulbColor);
        _tft->drawPixel(x + 8, y - 9, bulbColor);         // Top-right
        _tft->drawPixel(x + 7, y - 8, bulbColor);
    }
}

void PageMain::drawAmsIcon(int x, int y, uint16_t color) {
    _tft->drawRoundRect(x - 14, y - 8, 29, 16, 3, color);
    _tft->drawCircle(x - 7, y, 4, color);
    _tft->drawCircle(x + 7, y, 4, color);
}

void PageMain::drawPrintIcon(int x, int y, uint16_t color) {
    _tft->fillRect(x - 6, y - 8, 13, 10, color);
    _tft->fillTriangle(x - 6, y + 2, x + 6, y + 2, x, y + 8, color);
    _tft->drawFastHLine(x - 4, y - 10, 9, color);
}

void PageMain::onEnter() {
    _tft->fillScreen(_tft->color565(15, 23, 42)); // Background #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(15, 118, 110)); // Header #0f766e
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 118, 110));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString(config.activePrinter.name, 240, 25, 4);
    
    // Draw Light button
    _tft->fillRoundRect(15, 5, 50, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(15, 5, 50, 40, 8, TFT_WHITE);
    lastLightState = chamberLightOn;
    drawLightIcon(40, 25, TFT_WHITE, chamberLightOn);

    // Draw Notifications button
    _tft->fillRoundRect(355, 5, 50, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(355, 5, 50, 40, 8, TFT_WHITE);
    drawBellIcon(380, 25, TFT_WHITE, hmsHistory.size());

    // Draw custom gear icon for Settings button
    _tft->fillRoundRect(415, 5, 50, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(415, 5, 50, 40, 8, TFT_WHITE);
    drawGearIcon(440, 25, TFT_WHITE);
    
    _tft->setTextDatum(MC_DATUM);

    for(int i=0; i<4; i++) {
        lastAms[i].type = "FORCE_UPDATE";
        lastAms[i].color = "FORCE_UPDATE";
        lastRemain[i] = -2;
        lastBrand[i] = "FORCE_UPDATE";
    }
    
    // Draw Bottom Navigation Buttons
    _tft->fillRoundRect(10, 270, 90, 40, 8, _tft->color565(56, 189, 248)); // AMS is active
    _tft->drawRoundRect(10, 270, 90, 40, 8, TFT_WHITE);
    drawAmsIcon(32, 290, TFT_BLACK);
    _tft->setTextColor(TFT_BLACK, _tft->color565(56, 189, 248));
    _tft->drawString("AMS", 68, 290, 2);
    
    _tft->fillRoundRect(110, 270, 90, 40, 8, _tft->color565(30, 41, 59));  // Print is inactive
    _tft->drawRoundRect(110, 270, 90, 40, 8, TFT_WHITE);
    drawPrintIcon(132, 290, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->drawString("PRINT", 168, 290, 2);

    notificationDrawn = false;
    lastHumidity = "";
    drawLiveData();
}

void PageMain::drawLiveData() {
    // Light status update
    if (lastLightState != chamberLightOn) {
        lastLightState = chamberLightOn;
        _tft->fillRoundRect(15, 5, 50, 40, 8, _tft->color565(30, 41, 59));
        _tft->drawRoundRect(15, 5, 50, 40, 8, TFT_WHITE);
        drawLightIcon(40, 25, TFT_WHITE, chamberLightOn);
    }

    for(int i=0; i<4; i++) {
        if (lastAms[i].type != config.liveData.ams[i].type || lastAms[i].color != config.liveData.ams[i].color || lastRemain[i] != amsRemain[i] || lastBrand[i] != amsBrand[i]) {
            lastAms[i].type = config.liveData.ams[i].type;
            lastAms[i].color = config.liveData.ams[i].color;
            lastRemain[i] = amsRemain[i];
            lastBrand[i] = amsBrand[i];
            
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
            
            bool isBambu = false;
            String brandUpper = amsBrand[i];
            brandUpper.toUpperCase();
            const uint8_t* selectedLogo = logo_generic;
            
            for (int k = 0; k < sizeof(KNOWN_LOGOS)/sizeof(KNOWN_LOGOS[0]); k++) {
                String dictBrand = KNOWN_LOGOS[k].brand;
                dictBrand.toUpperCase();
                if (brandUpper.indexOf(dictBrand) >= 0) {
                    selectedLogo = KNOWN_LOGOS[k].bitmap;
                    if (dictBrand == "BAMBU") isBambu = true;
                    break;
                }
            }

            int pct = amsRemain[i];
            if (!isBambu) pct = 100; // Force visually full tank for Generic
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            
            int totalH = slots[i].h - 12;
            int remainH = (totalH * pct) / 100;
            int emptyH = totalH - remainH;

            _tft->fillRect(slots[i].x + 8, slots[i].y + 6, slots[i].w - 16, emptyH, _tft->color565(30, 41, 59));
            _tft->fillRect(slots[i].x + 8, slots[i].y + 6 + emptyH, slots[i].w - 16, remainH, color);
            
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
            yCursor -= 8; // Shift up slightly to leave room for percentage at bottom
            for (size_t j = 0; j < lines.size(); j++) {
                _tft->setTextColor(TFT_BLACK); // Drop-shadow
                _tft->drawString(lines[j], slots[i].x + slots[i].w/2 + 2, yCursor + 2, 4);
                _tft->setTextColor(TFT_WHITE); // Foreground
                _tft->drawString(lines[j], slots[i].x + slots[i].w/2, yCursor, 4);
                yCursor += 25;
            }
            
            // Draw drop-shadowed logo dynamically below the text
            _tft->drawXBitmap(slots[i].x + slots[i].w/2 - 8 + 1, yCursor + 2 + 1, selectedLogo, 16, 16, TFT_BLACK);
            _tft->drawXBitmap(slots[i].x + slots[i].w/2 - 8, yCursor + 2, selectedLogo, 16, 16, TFT_WHITE);

            if (isBambu) {
                _tft->setTextColor(TFT_BLACK);
                _tft->drawString(String(pct) + "%", slots[i].x + slots[i].w/2 + 2, slots[i].y + slots[i].h - 20 + 2, 2);
                _tft->setTextColor(TFT_WHITE);
                _tft->drawString(String(pct) + "%", slots[i].x + slots[i].w/2, slots[i].y + slots[i].h - 20, 2);
            }
        }
    }

    if (lastHumidity != config.liveData.amsHumidity) {
        lastHumidity = config.liveData.amsHumidity;
        
        _tft->fillRect(200, 260, 280, 60, _tft->color565(15, 23, 42)); // Clear footer strictly on right
        _tft->drawFastHLine(0, 260, 480, _tft->color565(51, 65, 85)); // Top border
        _tft->setTextColor(_tft->color565(148, 163, 184), _tft->color565(15, 23, 42));
        
        String humPercent = "0%";
        // If length > 1, it's definitely a raw percentage. Also catch "0" exactly.
        if (lastHumidity.length() > 1 || lastHumidity == "0") {
             humPercent = lastHumidity + "%";
        } else {
            if (lastHumidity == "1") humPercent = "Level 1";
            else if (lastHumidity == "2") humPercent = "Level 2";
            else if (lastHumidity == "3") humPercent = "Level 3";
            else if (lastHumidity == "4") humPercent = "Level 4";
            else if (lastHumidity == "5") humPercent = "Level 5";
            else humPercent = lastHumidity + "%";
        }
        
        _tft->setTextDatum(ML_DATUM);
        drawDropIcon(250, 288, _tft->color565(56, 189, 248)); // #38bdf8 light blue
        _tft->drawString("Humidity: " + humPercent, 265, 290, 4);
        _tft->setTextDatum(MC_DATUM);
    }
}

void PageMain::onUpdate() {
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        if (!notificationDrawn) {
            drawLiveData();
        }
    }

    if (hasNewHmsError) {
        hasNewHmsError = false;
        notificationDrawn = true;
        notificationTime = millis();
        
        // Draw floating toast banner
        _tft->fillRoundRect(40, 60, 400, 40, 8, _tft->color565(220, 38, 38));
        _tft->setTextColor(TFT_WHITE, _tft->color565(220, 38, 38));
        _tft->setTextDatum(MC_DATUM);
        String code = hmsHistory.empty() ? "" : hmsHistory.back().code;
        HMSDictEntry entry = getHMSEntry(code);
        _tft->drawString("Error " + code + ": " + entry.shortDesc, 240, 80, 2);
        
        // Redraw bell with new notification count
        _tft->fillRoundRect(355, 5, 50, 40, 8, _tft->color565(30, 41, 59));
        _tft->drawRoundRect(355, 5, 50, 40, 8, TFT_WHITE);
        drawBellIcon(380, 25, TFT_WHITE, hmsHistory.size());
        
    } else if (notificationDrawn && (millis() - notificationTime > 5000)) {
        notificationDrawn = false;
        onEnter(); // Redraw whole screen to cleanly erase the floating banner
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
        if (x > 355 && x < 405 && y < 60) { // Notification Bell
            _manager->switchPage(static_cast<AppState>(STATE_NOTIFICATIONS));
            return;
        }
        if (x > 400 && y < 60) { // Top right corner (Settings)
            _manager->switchPage(STATE_SETTINGS);
            return;
        }
        if (x >= 110 && x <= 200 && y >= 270 && y <= 310) { // PRINT Button
            _manager->switchPage(static_cast<AppState>(STATE_PRINT_MONITOR));
            return;
        }
    }
}