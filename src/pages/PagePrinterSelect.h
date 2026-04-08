#pragma once
#include "Page.h"
#include <vector>

class PagePrinterSelect : public Page {
public:
    PagePrinterSelect(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    std::vector<Button> printerButtons;
    Button btnReset;
    bool isFetching;
    void drawPrinterList();
};