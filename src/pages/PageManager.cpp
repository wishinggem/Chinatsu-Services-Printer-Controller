#include "PageManager.h"
#include "PageWifi.h"
#include "PageLogin.h"
#include "PagePrinterSelect.h"
#include "PageMain.h"
#include "PageSettings.h"

PageManager::PageManager(TFT_eSPI* tft, TAMC_GT911* touch) {
    _tft = tft;
    _touch = touch;
    _currentPage = nullptr;
    _kbActive = false;
}

void PageManager::begin() {
    _tft->fillScreen(TFT_BLACK);
    _tft->setTextColor(TFT_WHITE);
    _tft->setTextDatum(MC_DATUM);
    switchPage(STATE_WIFI_SCAN);
}

bool PageManager::checkTouchRaw(int &x, int &y) {
    static unsigned long lastTriggerTime = 0;
    static bool isHolding = false; 

    _touch->read();
    
    if (_touch->isTouched) {
        if (isHolding) return false; 
        isHolding = true;
        
        config.lastTouchTime = millis(); // Wake up screen & reset timeout!

        if (millis() - lastTriggerTime < 250) return false;
        lastTriggerTime = millis();
        
        int rawX = _touch->points[0].x;
        int rawY = _touch->points[0].y;
        
        x = rawY;          
        y = 320 - rawX;    
        
        bool FLIP_X = true; 
        bool FLIP_Y = true; 
        if (FLIP_X) x = 480 - x;
        if (FLIP_Y) y = 320 - y;

        if (x < 0) x = 0; if (x > 480) x = 480;
        if (y < 0) y = 0; if (y > 320) y = 320;
        
        return true;
    } else {
        isHolding = false;
        return false;
    }
}

bool PageManager::readTouch(int &x, int &y) {
    if (_kbActive) return false; 
    return checkTouchRaw(x, y);
}

void PageManager::update() {
    if (_currentPage) {
        _currentPage->onUpdate();
    }

    if (_kbActive) {
        int x, y;
        if (checkTouchRaw(x, y)) {
            updateKeyboard(x, y);
        }
    }
}

void PageManager::switchPage(AppState newState) {
    if (_currentPage) {
        _currentPage->onExit();
        delete _currentPage; 
        _currentPage = nullptr;
    }

    config.currentState = newState;
    _kbActive = false; 
    _tft->fillScreen(TFT_BLACK); 

    switch (newState) {
        case STATE_WIFI_SCAN: _currentPage = new PageWifi(_tft, _touch, this); break;
        case STATE_LOGIN: _currentPage = new PageLogin(_tft, _touch, this); break;
        case STATE_PRINTER_SELECT: _currentPage = new PagePrinterSelect(_tft, _touch, this); break; 
        case STATE_MAIN_PAGE: _currentPage = new PageMain(_tft, _touch, this); break;
        case STATE_SETTINGS: _currentPage = new PageSettings(_tft, _touch, this); break;
    }

    if (_currentPage) {
        _currentPage->onEnter();
    } 
}

bool PageManager::isKeyboardOpen() {
    return _kbActive;
}

void PageManager::openKeyboard(String* target) {
    _kbActive = true;
    _kbTarget = target;
    _kbBuffer = *target;
    _kbLayout = 0;
    
    _tft->fillRect(0, 130, 480, 190, TFT_BLACK); 
    buildKeyboard();
    drawKeyboard();
}

void PageManager::closeKeyboard(bool save) {
    _kbActive = false;
    if (save && _kbTarget) {
        *_kbTarget = _kbBuffer;
    }
    _tft->fillScreen(TFT_BLACK);
    if (_currentPage) _currentPage->onEnter();
}

void PageManager::buildKeyboard() {
    _kbKeys.clear();
    int kh = 32; int sp = 4; int kw = 43;
    int y1 = 170, y2 = 206, y3 = 242, y4 = 278;

    if (_kbLayout == 0) { // LOWERCASE
        String r1[] = {"q","w","e","r","t","y","u","i","o","p"};
        for(int i=0; i<10; i++) _kbKeys.push_back({5 + i*(kw+sp), y1, kw, kh, r1[i], TFT_DARKGREY});
        String r2[] = {"a","s","d","f","g","h","j","k","l"};
        for(int i=0; i<9; i++) _kbKeys.push_back({28 + i*(kw+sp), y2, kw, kh, r2[i], TFT_DARKGREY});
        
        _kbKeys.push_back({5, y3, 55, kh, "UP", TFT_BLUE});
        String r3[] = {"z","x","c","v","b","n","m"};
        for(int i=0; i<7; i++) _kbKeys.push_back({65 + i*(kw+sp), y3, kw, kh, r3[i], TFT_DARKGREY});
        _kbKeys.push_back({398, y3, 75, kh, "DEL", TFT_RED});

    } else if (_kbLayout == 1) { // UPPERCASE
        String r1[] = {"Q","W","E","R","T","Y","U","I","O","P"};
        for(int i=0; i<10; i++) _kbKeys.push_back({5 + i*(kw+sp), y1, kw, kh, r1[i], TFT_DARKGREY});
        String r2[] = {"A","S","D","F","G","H","J","K","L"};
        for(int i=0; i<9; i++) _kbKeys.push_back({28 + i*(kw+sp), y2, kw, kh, r2[i], TFT_DARKGREY});
        
        _kbKeys.push_back({5, y3, 55, kh, "low", TFT_BLUE});
        String r3[] = {"Z","X","C","V","B","N","M"};
        for(int i=0; i<7; i++) _kbKeys.push_back({65 + i*(kw+sp), y3, kw, kh, r3[i], TFT_DARKGREY});
        _kbKeys.push_back({398, y3, 75, kh, "DEL", TFT_RED});

    } else if (_kbLayout == 2) { // NUMBERS & COMMON SYMBOLS
        String r1[] = {"1","2","3","4","5","6","7","8","9","0"};
        for(int i=0; i<10; i++) _kbKeys.push_back({5 + i*(kw+sp), y1, kw, kh, r1[i], TFT_DARKGREY});
        String r2[] = {"-","/",":",";","(",")","$","&","@","\""};
        for(int i=0; i<10; i++) _kbKeys.push_back({5 + i*(kw+sp), y2, kw, kh, r2[i], TFT_DARKGREY});
        
        _kbKeys.push_back({5, y3, 55, kh, "#+=", TFT_BLUE});
        String r3[] = {".",",","?","!","'","*","%"};
        for(int i=0; i<7; i++) _kbKeys.push_back({65 + i*(kw+sp), y3, kw, kh, r3[i], TFT_DARKGREY});
        _kbKeys.push_back({398, y3, 75, kh, "DEL", TFT_RED});
        
    } else if (_kbLayout == 3) { // ADVANCED SYMBOLS
        String r1[] = {"[","]","{","}","#","%","^","*","+","="};
        for(int i=0; i<10; i++) _kbKeys.push_back({5 + i*(kw+sp), y1, kw, kh, r1[i], TFT_DARKGREY});
        String r2[] = {"_","\\","|","~","<",">","`","\xA3","\xA5"};
        for(int i=0; i<9; i++) _kbKeys.push_back({28 + i*(kw+sp), y2, kw, kh, r2[i], TFT_DARKGREY});
        
        _kbKeys.push_back({5, y3, 55, kh, "123", TFT_BLUE});
        String r3[] = {".",",","?","!","'"};
        for(int i=0; i<5; i++) _kbKeys.push_back({65 + i*(kw+sp), y3, kw, kh, r3[i], TFT_DARKGREY});
        _kbKeys.push_back({398, y3, 75, kh, "DEL", TFT_RED});
    }

    // BOTTOM ROW
    if (_kbLayout == 0 || _kbLayout == 1) {
        _kbKeys.push_back({5, y4, 80, kh, "123", TFT_BLUE});
    } else {
        _kbKeys.push_back({5, y4, 80, kh, "abc", TFT_BLUE});
    }
    
    _kbKeys.push_back({90, y4, 190, kh, "SPACE", TFT_DARKGREY});
    _kbKeys.push_back({285, y4, 90, kh, "CANC", TFT_ORANGE});
    _kbKeys.push_back({380, y4, 95, kh, "ENT", TFT_DARKGREEN});
}

// NEW HELPER FUNCTION
void PageManager::drawInputBox() {
    _tft->fillRect(5, 135, 470, 30, TFT_WHITE);
    _tft->setTextColor(TFT_BLACK, TFT_WHITE);
    _tft->drawString(_kbBuffer, 240, 150, 2);
}

void PageManager::drawKeyboard() {
    drawInputBox(); // Draw the white box
    
    // Draw all the keys
    for(auto& k : _kbKeys) {
        _tft->fillRect(k.x, k.y, k.w, k.h, k.color);
        _tft->drawRect(k.x, k.y, k.w, k.h, TFT_WHITE);
        _tft->setTextColor(TFT_WHITE, k.color);
        _tft->drawString(k.label, k.x + (k.w/2), k.y + (k.h/2), 2);
    }
}

void PageManager::updateKeyboard(int x, int y) {
    bool layoutChanged = false; // Flag to track if we need a full screen wipe

    for(auto& k : _kbKeys) {
        if (k.isTouched(x, y)) {
            if (k.label == "ENT") {
                closeKeyboard(true);
                return;
            } else if (k.label == "CANC") {
                closeKeyboard(false);
                return;
            } else if (k.label == "DEL") {
                if (_kbBuffer.length() > 0) _kbBuffer.remove(_kbBuffer.length() - 1);
            } else if (k.label == "SPACE") {
                _kbBuffer += " ";
            } else if (k.label == "UP") {
                _kbLayout = 1; buildKeyboard(); layoutChanged = true;
            } else if (k.label == "low" || k.label == "abc") {
                _kbLayout = 0; buildKeyboard(); layoutChanged = true;
            } else if (k.label == "123") {
                _kbLayout = 2; buildKeyboard(); layoutChanged = true;
            } else if (k.label == "#+=") {
                _kbLayout = 3; buildKeyboard(); layoutChanged = true;
            } else {
                _kbBuffer += k.label;
            }
            
            if (layoutChanged) {
                // If the user hit a layout swap button, do a full clear and redraw
                _tft->fillRect(0, 130, 480, 190, TFT_BLACK); 
                drawKeyboard();
            } else {
                // If they just typed a normal letter, ONLY redraw the white text box!
                drawInputBox(); 
            }
            break;
        }
    }
}