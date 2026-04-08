#include "PageAMS.h"
#include "PageManager.h"
#include "../BambuMQTT.h"

extern BambuMQTT printerMqtt;

PageAMS::PageAMS(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    btnBack = {20, 260, 180, 50, "Back", TFT_RED};
    btnSync = {220, 260, 240, 50, "SYNC SLOT", TFT_DARKGREEN};
    
    typeInput = {20, 80, 200, 50, "Type", &config.liveData.ams[config.activeAmsSlot].type};
    colorInput = {20, 160, 200, 50, "Color", &config.liveData.ams[config.activeAmsSlot].color};
    
    int pX = 240, pY = 50, pW = 50, pH = 40, spacing = 8;
    for (int i=0; i<16; i++) {
        int row = i / 4;
        int col = i % 4;
        palette[i] = {pX + col*(pW+spacing), pY + row*(pH+spacing), pW, pH, "", 0}; 
    }
}

uint16_t PageAMS::hexToRGB565(String hex) {
    if (hex.length() < 6) return TFT_WHITE;
    long rgb = strtol(hex.substring(0,6).c_str(), NULL, 16);
    return _tft->color565((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

void PageAMS::onEnter() {
    _tft->drawString("Editing Slot " + String(config.activeAmsSlot + 1), SCREEN_WIDTH/2, 20, 4);
    drawUI();
    _tft->fillRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, btnBack.color);
    _tft->drawString(btnBack.label, btnBack.x + (btnBack.w/2), btnBack.y + (btnBack.h/2), 4);
    
    _tft->fillRect(btnSync.x, btnSync.y, btnSync.w, btnSync.h, btnSync.color);
    _tft->drawString(btnSync.label, btnSync.x + (btnSync.w/2), btnSync.y + (btnSync.h/2), 4);
}

void PageAMS::drawUI() {
    _tft->fillRect(typeInput.x, typeInput.y, typeInput.w, typeInput.h, TFT_WHITE);
    _tft->setTextColor(TFT_BLACK);
    _tft->drawString(*typeInput.valuePtr, typeInput.x + typeInput.w/2, typeInput.y + 25, 4);
    
    _tft->fillRect(colorInput.x, colorInput.y, colorInput.w, colorInput.h, TFT_WHITE);
    _tft->drawString("#" + *colorInput.valuePtr, colorInput.x + colorInput.w/2, colorInput.y + 25, 4);
    _tft->setTextColor(TFT_WHITE);
    
    // Draw Quick Palette
    for (int i=0; i<16; i++) {
        _tft->fillRect(palette[i].x, palette[i].y, palette[i].w, palette[i].h, hexToRGB565(hexColors[i]));
        _tft->drawRect(palette[i].x, palette[i].y, palette[i].w, palette[i].h, TFT_WHITE);
    }
}

void PageAMS::onUpdate() {
    int x, y;
    if (_manager->readTouch(x, y)) {
        if (btnBack.isTouched(x, y)) {
            _manager->switchPage(STATE_MAIN_PAGE);
            return;
        }
        
        if (typeInput.isTouched(x, y)) {
            _manager->openKeyboard(typeInput.valuePtr); return;
        }
        if (colorInput.isTouched(x, y)) {
            _manager->openKeyboard(colorInput.valuePtr); return;
        }
        if (btnSync.isTouched(x, y)) {
            printerMqtt.sendAMSCommand(0, config.activeAmsSlot, config.liveData.ams[config.activeAmsSlot].type, config.liveData.ams[config.activeAmsSlot].color);
            _tft->fillRect(btnSync.x, btnSync.y, btnSync.w, btnSync.h, TFT_BLUE);
            _tft->drawString("Command Sent!", btnSync.x + btnSync.w/2, btnSync.y + 25, 4);
            delay(500);
            _tft->fillRect(btnSync.x, btnSync.y, btnSync.w, btnSync.h, btnSync.color);
            _tft->drawString(btnSync.label, btnSync.x + btnSync.w/2, btnSync.y + 25, 4);
            return;
        }
        
        for (int i=0; i<16; i++) {
            if (palette[i].isTouched(x, y)) {
                config.liveData.ams[config.activeAmsSlot].color = hexColors[i] + "FF"; // Bambu suffix
                drawUI(); 
                delay(200); 
            }
        }
    }
}