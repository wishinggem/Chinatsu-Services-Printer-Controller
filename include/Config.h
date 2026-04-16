#pragma once
#include <Arduino.h>
#include <vector>

#define TOUCH_SDA 33
#define TOUCH_SCL 32
#define TOUCH_INT 21
#define TOUCH_RST 25
#define TFT_BL_PIN 27

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

enum AppState {
    STATE_WIFI_SCAN,
    STATE_LOGIN,
    STATE_PRINTER_SELECT,
    STATE_MAIN_PAGE,
    STATE_SETTINGS
};

struct PrinterInfo {
    String name;
    String ip;
    String serial;
    String accessCode;
};

struct AMSTray {
    String color = "";
    String type = "";
};

struct PrinterLiveData {
    float nozzleTemp = 0;
    float nozzleTarget = 0;
    float bedTemp = 0;
    float bedTarget = 0;
    int progress = 0;
    String status = "OFFLINE";
    AMSTray ams[4]; // Store up to 4 slots
    String amsHumidity = "0"; 
    String amsDryTime = "0";
    String amsDryTemp = "0";
};

struct AppConfig {
    AppState currentState;
    PrinterInfo activePrinter;
    std::vector<PrinterInfo> printerList; 
    PrinterLiveData liveData;             
    
    String cloudToken;
    String userEmail;
    String userPassword; // <-- NEW
    String authCode;
    String tfaKey;       // <-- NEW
    String wifiSSID;
    String wifiPass;
    String userId;
    
    int brightness = 255;
    int screenTimeout = 5; // minutes (0 = never)
    unsigned long lastTouchTime = 0; 
    int activeAmsSlot = 0; // Track which slot is being edited
    
    AppState returnState;
    String* targetString;
    String tempInputBuffer;
};

extern AppConfig config;
extern void saveSettings(); 
extern void factoryReset(); // <-- NEW

struct Button {
    int x, y, w, h;
    String label;
    uint16_t color;
    
    bool isTouched(int touchX, int touchY) {
        return (touchX > x && touchX < x + w && touchY > y && touchY < y + h);
    }
};

struct TextInput {
    int x, y, w, h;
    String label;
    String* valuePtr;
    
    bool isTouched(int touchX, int touchY) {
        return (touchX > x && touchX < x + w && touchY > y && touchY < y + h);
    }
};