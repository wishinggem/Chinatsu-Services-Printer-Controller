#pragma once
#include "Page.h"
#include <vector>

class PageNotifications : public Page {
public:
    PageNotifications(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:

    int scrollOffset = 0;
    int selectedErrorIdx = -1;
    unsigned long lastTouchTime = 0;
    int lastTouchY = 0;
    bool isScrolling = false;
    
    void drawList();
    void drawPopup();
};