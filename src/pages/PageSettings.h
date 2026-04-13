#pragma once
#include "Page.h"

class PageSettings : public Page {
public:
    PageSettings(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;
    
private:
    Button btnBack, btnWipe, btnSwitchPrinter, btnLogout, btnSelectWifi;
    Button btnBrightUp, btnBrightDown, btnTimeUp, btnTimeDown;
    Button btnConfirmWipe, btnCancelWipe, btnSetIP, btnSetCode;

    void drawValues();
    void drawModal();
    bool _showModal;
};