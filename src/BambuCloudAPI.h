#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include "Config.h"

class BambuCloudAPI {
public:
    BambuCloudAPI();
    
    void setAuthToken(String token);
    bool hasAuthToken();

    // The Unified 2-Step Auth Flow
    int login(String email, String password, String& outTfaKey); 
    bool submitTfaCode(String tfaKey, String code);              
    
    // Required internally and for the linker
    bool requestEmailCode(String email);
    bool loginWithCode(String email, String code);
    
    bool getUserInfo(String& outUid);
    bool fetchPrinters(std::vector<PrinterInfo>& printers);

private:
    String _apiBase = "https://api.bambulab.com";
    String _authToken;
    
    void applyBambuHeaders(HTTPClient& https);
    int sendAuthenticatedGet(String endpoint, String& outPayload);
};