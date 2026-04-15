#include "PageOTA.h"
#include "PageManager.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

extern AsyncWebServer server;
static bool otaHandlersAttached = false;

TFT_eSPI* PageOTA::_ota_tft = nullptr;
bool PageOTA::_update_started = false;

PageOTA::PageOTA(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager)
    : Page(tft, touch, manager) {
    btnBack = {15, 270, 140, 40, "<- Back", _tft->color565(20, 83, 45)};
    _ota_tft = _tft; // Assign to static member
}

void PageOTA::onEnter() {
    _update_started = false;
    _tft->fillScreen(_tft->color565(15, 23, 42)); // #0f172a
    _tft->fillRect(0, 0, 480, 50, _tft->color565(30, 41, 59)); // Header #1e293b

    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_WHITE, _tft->color565(30, 41, 59));
    _tft->drawString("Firmware Update (OTA)", SCREEN_WIDTH / 2, 25, 4);

    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
    _tft->setTextDatum(TL_DATUM);
    _tft->drawString("1. Connect to this device's WiFi AP or", 20, 70, 2);
    _tft->drawString("   ensure you are on the same network.", 20, 90, 2);
    _tft->drawString("2. Open a web browser and go to:", 20, 120, 2);
    
    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(TFT_YELLOW, _tft->color565(15, 23, 42));
    _tft->drawString("http://" + WiFi.localIP().toString() + "/update", SCREEN_WIDTH / 2, 150, 4);

    _tft->setTextColor(TFT_WHITE, _tft->color565(15, 23, 42));
    _tft->setTextDatum(TL_DATUM);
    _tft->drawString("3. Upload the 'firmware.bin' file.", 20, 180, 2);
    _tft->drawString("The device will reboot automatically.", 20, 200, 2);

    // Draw Back button
    _tft->setTextDatum(MC_DATUM);
    _tft->fillRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, btnBack.color);
    _tft->drawRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, TFT_WHITE);
    _tft->setTextColor(TFT_WHITE, btnBack.color);
    _tft->drawString(btnBack.label, btnBack.x + btnBack.w / 2, btnBack.y + btnBack.h / 2, 4);

    // Set up the progress callback
    Update.onProgress(drawProgress);

    // Start the web server just for this page
    if (!otaHandlersAttached) {
        AsyncElegantOTA.begin(&server);
        otaHandlersAttached = true;
    }
    server.begin();
}

void PageOTA::onExit() {
    // Clear the progress callback when leaving the page
    Update.onProgress(nullptr);
    _ota_tft = nullptr;
    server.end(); // Stop the web server
}

void PageOTA::drawProgress(size_t progress, size_t total) {
    if (_ota_tft == nullptr) return;

    if (!_update_started) {
        _ota_tft->fillScreen(_ota_tft->color565(15, 23, 42));
        _ota_tft->setTextDatum(MC_DATUM);
        _ota_tft->setTextColor(TFT_WHITE, _ota_tft->color565(15, 23, 42));
        _ota_tft->drawString("Update in progress...", SCREEN_WIDTH / 2, 100, 4);
        _ota_tft->drawString("Do not power off the device!", SCREEN_WIDTH / 2, 130, 2);
        _update_started = true;
    }

    int percentage = (progress * 100) / total;
    
    _ota_tft->drawRect(40, 160, 400, 30, TFT_WHITE);
    _ota_tft->fillRect(45, 165, 390 * percentage / 100, 20, TFT_GREEN);
    _ota_tft->drawString(String(percentage) + "%", SCREEN_WIDTH / 2, 175, 4);
}

void PageOTA::onUpdate() {
    int x, y;
    if (_manager->readTouch(x, y)) {
        if (btnBack.isTouched(x, y)) {
            _manager->switchPage(STATE_SETTINGS);
        }
    }
}