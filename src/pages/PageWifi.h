#pragma once
#include "Page.h"
#include <vector>

class PageWifi : public Page {
public:
    PageWifi(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    enum WifiState { SCANNING, SELECTING, CONNECTING };
    
    WifiState _state;
    Button btnSkip;
    Button btnRefresh; 
    Button btnReset;
    std::vector<Button> networkButtons;
    
    int loadingDots;
    unsigned long lastDotTime;
    unsigned long connectStartTime;
    void drawNetworks();
};