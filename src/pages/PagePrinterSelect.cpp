#include "PagePrinterSelect.h"
#include "PageManager.h"
#include "../BambuCloudAPI.h"
#include "../BambuMQTT.h"

extern BambuCloudAPI cloudApi;
extern BambuMQTT printerMqtt;

PagePrinterSelect::PagePrinterSelect(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    isFetching = true;
    btnReset = {380, 5, 95, 30, "RESET", TFT_MAROON}; 
}

void PagePrinterSelect::onEnter() {
    _tft->drawString("Select Printer", SCREEN_WIDTH/2, 20, 4);
    
    _tft->fillRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, btnReset.color);
    _tft->drawRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, TFT_WHITE);
    _tft->drawString(btnReset.label, btnReset.x + (btnReset.w/2), btnReset.y + (btnReset.h/2), 2);
    
    _tft->drawString("Fetching devices from Bambu Cloud...", SCREEN_WIDTH/2, 160, 2);
    
    config.printerList.clear();
    printerButtons.clear();
}

void PagePrinterSelect::drawPrinterList() {
    _tft->fillRect(0, 60, 480, 260, TFT_BLACK); 
    
    if (config.printerList.size() == 0) {
        _tft->setTextColor(TFT_RED);
        _tft->drawString("No printers found on this account!", SCREEN_WIDTH/2, 160, 2);
        _tft->setTextColor(TFT_WHITE);
        return;
    }
    
    for (int i = 0; i < config.printerList.size(); i++) {
        String label = config.printerList[i].name + " (" + config.printerList[i].ip + ")";
        int yPos = 80 + (i * 65);
        printerButtons.push_back({40, yPos, 400, 50, label, TFT_DARKCYAN});
        
        _tft->fillRect(40, yPos, 400, 50, TFT_DARKCYAN);
        _tft->drawString(label, SCREEN_WIDTH/2, yPos + 25, 2);
    }
}

void PagePrinterSelect::onUpdate() {
    int x, y;
    bool touched = _manager->readTouch(x, y);
    
    if (touched && btnReset.isTouched(x, y)) {
        factoryReset(); return;
    }

    if (isFetching) {
        if (cloudApi.fetchPrinters(config.printerList)) {
            isFetching = false;
            drawPrinterList();
        } else {
            _tft->fillRect(0, 60, 480, 260, TFT_BLACK);
            _tft->setTextColor(TFT_RED);
            _tft->drawString("Failed to fetch printers. Retrying...", SCREEN_WIDTH/2, 160, 2);
            _tft->setTextColor(TFT_WHITE);
            delay(2000);
        }
        return;
    }

    if (touched) {
        for (int i = 0; i < printerButtons.size(); i++) {
            if (printerButtons[i].isTouched(x, y)) {
                config.activePrinter = config.printerList[i];
                saveSettings();
                
                _tft->fillRect(0, 60, 480, 260, TFT_BLACK);
                _tft->drawString("Connecting to MQTT...", SCREEN_WIDTH/2, 160, 4);
                
                printerMqtt.begin(config.activePrinter.serial, config.userId, config.cloudToken);
                _manager->switchPage(STATE_MAIN_PAGE);
                break;
            }
        }
    }
}