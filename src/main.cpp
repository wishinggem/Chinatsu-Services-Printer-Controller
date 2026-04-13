#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <TAMC_GT911.h>
#include "Config.h"
#include "pages/PageManager.h"
#include "BambuCloudAPI.h"
#include "BambuMQTT.h"

// ==========================================
// SETTINGS
// Change this to true, upload once to wipe, then change back to false!
#define WIPE_ON_FIRMWARE_UPLOAD false
// ==========================================

AppConfig config;
Preferences prefs;

TFT_eSPI tft = TFT_eSPI();
TAMC_GT911 touch = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, 320, 480);

PageManager uiManager(&tft, &touch);
BambuCloudAPI cloudApi;
BambuMQTT printerMqtt;

void factoryReset() {
    Serial.println("\n[!] FACTORY RESET INITIATED FROM UI...");
    prefs.begin("panda", false);
    prefs.clear(); 
    prefs.end();
    delay(500); 
    ESP.restart(); 
}

void saveSettings() {
    Serial.println("Saving settings to NVS...");
    prefs.begin("panda", false);
    
    prefs.putBool("setupDone", true);
    prefs.putString("email", config.userEmail);
    prefs.putString("auth", config.authCode);
    prefs.putString("ssid", config.wifiSSID);
    prefs.putString("pass", config.wifiPass);
    prefs.putString("token", config.cloudToken);
    prefs.putString("uid", config.userId);
    prefs.putInt("bright", config.brightness);
    prefs.putInt("timeout", config.screenTimeout);
    
    if (config.activePrinter.serial != "") {
        prefs.putString("p_name", config.activePrinter.name);
        prefs.putString("p_ip", config.activePrinter.ip);
        prefs.putString("p_serial", config.activePrinter.serial);
        prefs.putString("p_code", config.activePrinter.accessCode);
    }
    
    prefs.end();
}

void setup() {
    Serial.begin(115200);
    
    // Let analogWrite handle the PWM backlight
    analogWrite(TFT_BL_PIN, 255);
    
    tft.init();
    tft.setRotation(1); 
    
    touch.begin();
    touch.setRotation(ROTATION_NORMAL); 

    config.userEmail = "";
    config.authCode = "";
    config.wifiSSID = "";
    config.wifiPass = "";

    uiManager.begin(); 

    prefs.begin("panda", false);
    
    if (WIPE_ON_FIRMWARE_UPLOAD) {
        String currentBuild = String(__DATE__) + " " + String(__TIME__);
        String savedBuild = prefs.getString("build_time", "");
        
        if (currentBuild != savedBuild) {
            Serial.println("\n[!] WIPE FLAG IS TRUE AND NEW FIRMWARE DETECTED.");
            prefs.clear(); 
            prefs.putString("build_time", currentBuild); 
        }
    }

    bool isSetup = prefs.getBool("setupDone", false);
    
    if (isSetup) {
        Serial.println("Previous setup found! Connecting to WiFi...");
        
        config.userEmail = prefs.getString("email", "");
        config.authCode = prefs.getString("auth", "");
        config.wifiSSID = prefs.getString("ssid", "");
        config.wifiPass = prefs.getString("pass", "");
        config.cloudToken = prefs.getString("token", "");
        config.userId = prefs.getString("uid", "");
        
        config.brightness = prefs.getInt("bright", 255);
        config.screenTimeout = prefs.getInt("timeout", 5);
        config.lastTouchTime = millis();
        
        config.activePrinter.name = prefs.getString("p_name", "");
        config.activePrinter.ip = prefs.getString("p_ip", "");
        config.activePrinter.serial = prefs.getString("p_serial", "");
        config.activePrinter.accessCode = prefs.getString("p_code", "");

        cloudApi.setAuthToken(config.cloudToken);
        
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Connecting to WiFi...", 240, 160, 4);
        Serial.printf("Attempting to connect to SSID: '%s'\n", config.wifiSSID.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false); // Disable power saving to improve connection stability and speed
        WiFi.disconnect();
        delay(500);
        WiFi.setAutoReconnect(true); // Enhanced robustness
        WiFi.begin(config.wifiSSID.c_str(), config.wifiPass.c_str());
        
        Serial.print("Waiting for WiFi...");
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 40) { // 20 second timeout
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(" Connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            
            if (config.activePrinter.serial != "") {
                uiManager.switchPage(STATE_MAIN_PAGE);
                printerMqtt.begin(config.activePrinter.serial, config.userId, config.cloudToken);
            } else {
                uiManager.switchPage(STATE_PRINTER_SELECT);
            }
        } else {
            Serial.println(" FAILED!");
            Serial.println("Could not connect to WiFi. Falling back to WiFi Scan page.");
            uiManager.switchPage(STATE_WIFI_SCAN);
        }
    } else {
        Serial.println("First boot or wiped. Starting WiFi Scan.");
        uiManager.switchPage(STATE_WIFI_SCAN);
    }
    prefs.end();
}

void loop() {
    uiManager.update();
    
    // Screen Timeout Logic
    static int currentBrightness = -1;
    int targetBrightness = config.brightness;
    
    if (config.screenTimeout > 0 && millis() - config.lastTouchTime > (config.screenTimeout * 60000UL)) {
        targetBrightness = 0; // Turn off screen
    }
    
    if (currentBrightness != targetBrightness) {
        currentBrightness = targetBrightness;
        analogWrite(TFT_BL_PIN, currentBrightness); 
    }
    
    // Keep MQTT and WiFi alive on ALL pages (as long as we are past the setup screens)
    if (config.currentState != STATE_WIFI_SCAN && 
        config.currentState != STATE_LOGIN && 
        config.currentState != STATE_PRINTER_SELECT) {
        static unsigned long lastWifiCheck = 0;
        if (WiFi.status() != WL_CONNECTED) {
            if (millis() - lastWifiCheck > 10000) { // 10 second retry timer
                lastWifiCheck = millis();
                Serial.println("[WiFi] Connection dropped! Forcing reconnect...");
                WiFi.disconnect();
                WiFi.begin(config.wifiSSID.c_str(), config.wifiPass.c_str());
            }
        } else {
            lastWifiCheck = millis(); // Keep timer reset while connected
            printerMqtt.loop();
        }
    }
}