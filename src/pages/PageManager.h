#pragma once
#include "Page.h"
#include <vector>

class PageManager {
public:
    PageManager(TFT_eSPI* tft, TAMC_GT911* touch);
    void begin();
    void update();
    void switchPage(AppState newState);
    
    // Popup Controls
    void openKeyboard(String* target);
    void closeKeyboard(bool save);
    bool isKeyboardOpen();
    
    // Touch Reader for Pages
    bool readTouch(int &x, int &y);

private:
    TFT_eSPI* _tft;
    TAMC_GT911* _touch;
    Page* _currentPage;
    
    // Internal raw touch with debounce
    bool checkTouchRaw(int &x, int &y);

    // Keyboard Overlay State
    bool _kbActive;
    String* _kbTarget;
    String _kbBuffer;
    int _kbLayout; 
    std::vector<Button> _kbKeys;
    
    void buildKeyboard();
    void drawKeyboard();
    void drawInputBox(); // <-- NEW HELPER
    void updateKeyboard(int x, int y);
};