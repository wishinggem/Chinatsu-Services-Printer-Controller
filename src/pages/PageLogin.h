#pragma once
#include "Page.h"

enum LoginState { INPUT_PASS, INPUT_APP_CODE, INPUT_EMAIL_CODE };

class PageLogin : public Page {
public:
    PageLogin(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    Button btnLogin;
    Button btnReset;
    TextInput inputEmail;
    TextInput inputPassword;
    TextInput inputAuthCode;
    
    LoginState _loginState;
    void drawUI();
};