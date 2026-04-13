#pragma once
#include "Page.h"

class PageAMS : public Page {
public:
    PageAMS(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    Button btnBack;
    Button btnSync;
    TextInput typeInput;
    TextInput colorInput;
    Button palette[16];

    String hexColors[16] = {
        "FFFFFF", "000000", "888888", "444444",
        "FF0000", "FFA500", "FFFF00", "90EE90",
        "008000", "00FFFF", "ADD8E6", "0000FF",
        "800080", "FF00FF", "FFC0CB", "A52A2A"
    };
    uint16_t hexToRGB565(String hex);
    void drawUI();
};