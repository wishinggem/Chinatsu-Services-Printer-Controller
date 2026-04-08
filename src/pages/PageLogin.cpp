#include "PageLogin.h"
#include "PageManager.h"
#include "../BambuCloudAPI.h"

extern BambuCloudAPI cloudApi;

PageLogin::PageLogin(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager) 
    : Page(tft, touch, manager) {
    
    _loginState = INPUT_PASS;
    
    inputEmail = {20, 60, 440, 50, "Email", &config.userEmail};
    inputPassword = {20, 130, 440, 50, "Password", &config.userPassword};
    inputAuthCode = {20, 130, 440, 50, "2FA Code", &config.authCode}; // Same position, swapped out dynamically
    
    btnReset = {380, 5, 95, 30, "RESET", TFT_MAROON}; 
    btnLogin = {140, 220, 200, 60, "LOGIN", TFT_BLUE};
}

void PageLogin::onEnter() {
    _tft->fillScreen(TFT_BLACK);
    drawUI();
}

void PageLogin::drawUI() {
    _tft->fillRect(0, 0, SCREEN_WIDTH, 40, TFT_BLACK); // Clear header
    _tft->drawString("Bambu Cloud Login", SCREEN_WIDTH/2, 20, 4);
    
    _tft->fillRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, btnReset.color);
    _tft->drawRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, TFT_WHITE);
    _tft->drawString(btnReset.label, btnReset.x + (btnReset.w/2), btnReset.y + (btnReset.h/2), 2);
    
    // Always draw Email
    _tft->fillRect(inputEmail.x, inputEmail.y, inputEmail.w, inputEmail.h, TFT_WHITE);
    _tft->setTextColor(TFT_BLACK);
    String emailDisplay = (*inputEmail.valuePtr == "") ? "Tap to enter Email" : *inputEmail.valuePtr;
    _tft->drawString(emailDisplay, inputEmail.x + (inputEmail.w/2), inputEmail.y + 25, 2);
    
    // State Logic
    if (_loginState == INPUT_PASS) {
        _tft->fillRect(inputPassword.x, inputPassword.y, inputPassword.w, inputPassword.h, TFT_WHITE);
        String passDisplay = "";
        for(int i=0; i<config.userPassword.length(); i++) passDisplay += "*"; // Mask password
        if (passDisplay == "") passDisplay = "Tap to enter Password";
        _tft->drawString(passDisplay, inputPassword.x + (inputPassword.w/2), inputPassword.y + 25, 2);
        
        btnLogin.label = "LOGIN";
    } 
    else {
        _tft->fillRect(inputAuthCode.x, inputAuthCode.y, inputAuthCode.w, inputAuthCode.h, TFT_WHITE);
        String authDisplay = (*inputAuthCode.valuePtr == "") ? "Tap to enter 2FA Code" : *inputAuthCode.valuePtr;
        _tft->drawString(authDisplay, inputAuthCode.x + (inputAuthCode.w/2), inputAuthCode.y + 25, 2);
        
        btnLogin.label = "SUBMIT CODE";
    }
    
    _tft->setTextColor(TFT_WHITE); 
    _tft->fillRect(btnLogin.x, btnLogin.y, btnLogin.w, btnLogin.h, btnLogin.color);
    _tft->drawString(btnLogin.label, btnLogin.x + (btnLogin.w/2), btnLogin.y + (btnLogin.h/2), 4);
}

void PageLogin::onUpdate() {
    int x, y;
    if (_manager->readTouch(x, y)) {
        if (btnReset.isTouched(x, y)) {
            factoryReset(); return;
        }
        else if (inputEmail.isTouched(x, y)) {
            _manager->openKeyboard(inputEmail.valuePtr);
        }
        else if (_loginState == INPUT_PASS && inputPassword.isTouched(x, y)) {
            _manager->openKeyboard(inputPassword.valuePtr);
        }
        else if (_loginState != INPUT_PASS && inputAuthCode.isTouched(x, y)) {
            _manager->openKeyboard(inputAuthCode.valuePtr);
        }
        else if (btnLogin.isTouched(x, y)) {
            _tft->fillRect(btnLogin.x, btnLogin.y, btnLogin.w, btnLogin.h, TFT_DARKGREY);
            _tft->drawString("Connecting...", btnLogin.x + (btnLogin.w/2), btnLogin.y + (btnLogin.h/2), 4);
            
            bool finalSuccess = false;
            
            // 1. Initial Password Login
            if (_loginState == INPUT_PASS) {
                int result = cloudApi.login(config.userEmail, config.userPassword, config.tfaKey);
                
                if (result == 1) finalSuccess = true;
                else if (result == 2) {
                    _loginState = INPUT_APP_CODE;
                    _tft->fillScreen(TFT_BLACK); drawUI();
                    return; // Wait for user to type code
                }
                else if (result == 3) {
                    _loginState = INPUT_EMAIL_CODE;
                    _tft->fillScreen(TFT_BLACK); drawUI();
                    return; // Wait for user to type code
                }
            } 
            // 2. Submit Authenticator App Code
            else if (_loginState == INPUT_APP_CODE) {
                finalSuccess = cloudApi.submitTfaCode(config.tfaKey, config.authCode);
            }
            // 3. Submit Email Verification Code
            else if (_loginState == INPUT_EMAIL_CODE) {
                finalSuccess = cloudApi.loginWithCode(config.userEmail, config.authCode);
            }
            
            if (finalSuccess) {
                config.userPassword = ""; // Clear password from RAM for safety
                if (!cloudApi.getUserInfo(config.userId)) {
                    Serial.println("[CloudAPI] Warning: Failed to fetch User ID!");
                }
                saveSettings(); 
                _manager->switchPage(STATE_PRINTER_SELECT);
            } else {
                _tft->fillRect(btnLogin.x, btnLogin.y, btnLogin.w, btnLogin.h, TFT_RED);
                _tft->drawString("Failed!", btnLogin.x + (btnLogin.w/2), btnLogin.y + (btnLogin.h/2), 4);
                delay(2000);
                drawUI(); // Redraw normal button
            }
        }
    }
}