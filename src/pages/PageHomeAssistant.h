#pragma once
#include "Page.h"

class PageHomeAssistant : public Page {
public:
    PageHomeAssistant(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    Button btnBack;
    Button btnToggleSend;
    Button btnToggleRecv;
    Button btnDump;
    
    TextInput inputIp;
    TextInput inputPort;
    TextInput inputEndpoint;
    TextInput inputInterval;
    
    void drawUI();
};