#pragma once
#include "Page.h"
#include <Update.h>

class PageOTA : public Page {
public:
    PageOTA(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;
    void onExit() override;

    static void drawProgress(size_t progress, size_t total);

private:
    Button btnBack;
    static TFT_eSPI* _ota_tft;
    static bool _update_started;
};