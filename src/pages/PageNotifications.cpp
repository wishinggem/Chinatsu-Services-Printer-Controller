#include "PageNotifications.h"
#include "PageManager.h"
#include "../BambuMQTT.h"

PageNotifications::PageNotifications(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager)
    : Page(tft, touch, manager) {
}

void PageNotifications::onEnter() {
    selectedErrorIdx = -1;
    scrollOffset = 0;
    isScrolling = false;
    hasNewHmsError = false;
    
    Serial.println("\n[PageNotifications] Entering Notification Center...");
    Serial.printf("[PageNotifications] Total history items: %d\n", hmsHistory.size());

    _tft->fillScreen(_tft->color565(15, 23, 42)); // Background #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(15, 118, 110)); // Header #0f766e
    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 118, 110));
    _tft->setTextDatum(ML_DATUM);
    _tft->drawString("Notifications", 20, 25, 4);

    if (!hmsHistory.empty()) {
        // Clear All button (Only show if there are actual notifications)
        _tft->fillRoundRect(260, 5, 90, 40, 8, _tft->color565(220, 38, 38));
        _tft->drawRoundRect(260, 5, 90, 40, 8, TFT_WHITE);
        _tft->setTextColor(TFT_WHITE, _tft->color565(220, 38, 38));
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("CLEAR", 305, 25, 2);
    }

    // Back button - ALWAYS AVAILABLE
    _tft->fillRoundRect(370, 5, 90, 40, 8, _tft->color565(30, 41, 59));
    _tft->drawRoundRect(370, 5, 90, 40, 8, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("BACK", 415, 25, 2);

    drawList();
}

void PageNotifications::drawList() {
    _tft->fillRect(0, 50, 480, 270, _tft->color565(15, 23, 42));
    int y = 60;
    
    if (hmsHistory.empty()) {
        _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("No recent notifications", 240, 160, 4);
        return;
    }

    int maxVisible = 4;
    bool scrolling = hmsHistory.size() > maxVisible;
    int listWidth = scrolling ? 340 : 390;
    int startIdx = scrollOffset / 60;

    _tft->setTextDatum(ML_DATUM);
    for (int i = 0; i < maxVisible; i++) {
        int idx = startIdx + i;
        if (idx >= hmsHistory.size()) break;
        
        HMSDictEntry entry = getHMSEntry(hmsHistory[idx].code);
        _tft->fillRoundRect(10, y, listWidth, 50, 5, _tft->color565(30, 41, 59));
        
        _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
        _tft->drawString(hmsHistory[idx].code, 20, y + 16, 2);
        _tft->setTextColor(_tft->color565(148, 163, 184)); // Lighter gray for desc
        _tft->drawString(entry.shortDesc, 20, y + 36, 2);
        
        // Show how long ago the error was triggered
        unsigned long sec = (millis() - hmsHistory[idx].timestamp) / 1000;
        String timeStr;
        if (sec < 60) timeStr = String(sec) + "s";
        else if (sec < 3600) timeStr = String(sec / 60) + "m";
        else timeStr = String(sec / 3600) + "h";
        
        _tft->setTextDatum(TR_DATUM);
        _tft->drawString(timeStr, 10 + listWidth - 10, y + 16, 2);
        _tft->setTextDatum(ML_DATUM);
        
        // Delete button for individual
        _tft->fillRoundRect(10 + listWidth + 10, y, 60, 50, 5, _tft->color565(220, 38, 38));
        _tft->setTextColor(TFT_WHITE, _tft->color565(220, 38, 38));
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("DEL", 10 + listWidth + 10 + 30, y + 25, 2);
        _tft->setTextDatum(ML_DATUM);
        
        y += 60;
    }
    
    if (scrolling) {
        _tft->fillRoundRect(430, 60, 40, 100, 5, _tft->color565(51, 65, 85));
        _tft->drawRoundRect(430, 60, 40, 100, 5, TFT_WHITE);
        _tft->setTextColor(TFT_WHITE, _tft->color565(51, 65, 85));
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("/\\", 450, 110, 2); // UP
        
        _tft->fillRoundRect(430, 170, 40, 100, 5, _tft->color565(51, 65, 85));
        _tft->drawRoundRect(430, 170, 40, 100, 5, TFT_WHITE);
        _tft->drawString("\\/", 450, 220, 2); // DOWN
    }
}

void PageNotifications::drawPopup() {
    if (selectedErrorIdx < 0 || selectedErrorIdx >= hmsHistory.size()) return;
    
    HMSDictEntry entry = getHMSEntry(hmsHistory[selectedErrorIdx].code);
    
    Serial.printf("[PageNotifications] Opening popup for error: %s\n", hmsHistory[selectedErrorIdx].code.c_str());

    _tft->fillRoundRect(20, 40, 440, 240, 10, _tft->color565(51, 65, 85));
    _tft->drawRoundRect(20, 40, 440, 240, 10, TFT_WHITE);
    
    _tft->setTextColor(TFT_WHITE, _tft->color565(51, 65, 85));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("Error: " + hmsHistory[selectedErrorIdx].code, 240, 60, 4);
    
    _tft->setTextDatum(ML_DATUM);
    String desc = entry.longDesc;
    int cursorY = 90;
    int start = 0;
    while(start < desc.length() && cursorY < 260) {
        int end = start + 40; // approx 40 chars per line for full width
        if (end < desc.length()) {
            while (end > start && desc[end] != ' ') end--;
            if (end == start) end = start + 40;
        } else {
            end = desc.length();
        }
        _tft->drawString(desc.substring(start, end), 30, cursorY, 2);
        start = end + 1;
        cursorY += 20;
    }
    
    // Close button
    _tft->fillRoundRect(410, 50, 40, 30, 5, _tft->color565(220, 38, 38));
    _tft->drawRoundRect(410, 50, 40, 30, 5, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, _tft->color565(220, 38, 38));
    _tft->setTextDatum(MC_DATUM);
    _tft->drawString("X", 430, 65, 2);
}

void PageNotifications::onUpdate() {
    int x, y;
    if (_manager->readTouch(x, y)) {
        if (selectedErrorIdx >= 0) {
            // Check close popup
            if (x > 410 && x < 450 && y > 50 && y < 80) {
                selectedErrorIdx = -1;
                drawList();
            }
            return;
        }

        // Top bar buttons
        if (y < 50) {
            if (x > 370) {
                _manager->switchPage(STATE_MAIN_PAGE);
                return;
            }
            if (x > 260 && x < 360) {
                if (!hmsHistory.empty()) {
                    Serial.println("[PageNotifications] Clearing all notifications!");
                    hmsHistory.clear();
                    scrollOffset = 0;
                    onEnter(); // Redraws whole screen, hiding the clear button
                }
                return;
            }
        }

        // List clicks
        if (y >= 60 && y <= 300 && (millis() - lastTouchTime > 300)) {
            int maxVisible = 4;
            bool scrolling = hmsHistory.size() > maxVisible;
            int listWidth = scrolling ? 340 : 390;
            
            // Check scroll buttons
            if (scrolling) {
                if (y >= 60 && y <= 160) {
                    // UP
                    if (scrollOffset > 0) {
                        scrollOffset -= 60;
                        drawList();
                        lastTouchTime = millis();
                    }
                } else if (y >= 170 && y <= 270) {
                    // DOWN
                    int maxScroll = (hmsHistory.size() - maxVisible) * 60;
                    if (scrollOffset < maxScroll) {
                        scrollOffset += 60;
                        drawList();
                        lastTouchTime = millis();
                    }
                }
                return;
            }
            
            // Check item clicks
            int clickedRow = (y - 60) / 60;
            if (clickedRow >= 0 && clickedRow < maxVisible) {
                int clickedIdx = (scrollOffset / 60) + clickedRow;
                if (clickedIdx >= 0 && clickedIdx < hmsHistory.size()) {
                    if (x > 10 + listWidth && x < 10 + listWidth + 70) { // Delete
                        Serial.printf("[PageNotifications] Deleting notification: %s\n", hmsHistory[clickedIdx].code.c_str());
                        hmsHistory.erase(hmsHistory.begin() + clickedIdx);
                        // Adjust scroll if we deleted the last item causing empty space
                        int maxScroll = hmsHistory.size() > maxVisible ? (hmsHistory.size() - maxVisible) * 60 : 0;
                        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
                        
                        if (hmsHistory.empty()) {
                            onEnter(); // Gracefully hide the clear button if it was the last item
                        } else {
                            drawList();
                        }
                        lastTouchTime = millis();
                    } else if (x > 10 && x <= 10 + listWidth) { // Popup
                        selectedErrorIdx = clickedIdx;
                        drawPopup();
                        lastTouchTime = millis();
                    }
                }
            }
        }
    }
}