#pragma once
#include <TFT_eSPI.h>
#include <TAMC_GT911.h>
#include "Config.h"

class PageManager;

class Page {
protected:
    TFT_eSPI* _tft;
    TAMC_GT911* _touch;
    PageManager* _manager;

public:
    Page(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
        : _tft(tft), _touch(touch), _manager(manager) {}
    
    virtual ~Page() {}
    virtual void onEnter() = 0; 
    virtual void onUpdate() = 0; 
    virtual void onExit() {} 
};