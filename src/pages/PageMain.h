#pragma once
#include "Page.h"

class PageMain : public Page {
public:
    PageMain(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    Button slots[4];
    
    String lastHumidity;
    AMSTray lastAms[4];
    
    unsigned long lastUpdate;
    void drawLiveData();
    uint16_t hexToRGB565(String hex);
    void drawGearIcon(int x, int y, uint16_t color);
    void drawDropIcon(int x, int y, uint16_t color);
};