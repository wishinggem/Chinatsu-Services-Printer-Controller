#include "BambuCloudAPI.h"
#include <WiFiClientSecure.h>

BambuCloudAPI::BambuCloudAPI() {
    _authToken = "";
}

void BambuCloudAPI::setAuthToken(String token) {
    _authToken = token;
}

bool BambuCloudAPI::hasAuthToken() {
    return _authToken.length() > 0;
}

// Added hidden Python 'requests' headers to bypass Cloudflare 403 blocks
void BambuCloudAPI::applyBambuHeaders(HTTPClient& https) {
    https.setUserAgent("bambu_network_agent/01.09.05.01");
    https.addHeader("X-BBL-Client-Name", "OrcaSlicer");
    https.addHeader("X-BBL-Client-Type", "slicer");
    https.addHeader("X-BBL-Client-Version", "01.09.05.51");
    https.addHeader("X-BBL-Language", "en-US");
    https.addHeader("X-BBL-OS-Type", "linux");
    https.addHeader("X-BBL-OS-Version", "6.2.0");
    https.addHeader("X-BBL-Agent-Version", "01.09.05.01");
    https.addHeader("X-BBL-Executable-info", "{}");
    https.addHeader("X-BBL-Agent-OS-Type", "linux");
    https.addHeader("Accept", "application/json");
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Connection", "keep-alive");
}

int BambuCloudAPI::sendAuthenticatedGet(String endpoint, String& outPayload) {
    if (_authToken == "") return 401;

    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    client->setHandshakeTimeout(30000);
    HTTPClient https;

    int httpCode = 0;
    Serial.printf("[CloudAPI] GET Request -> %s\n", endpoint.c_str());
    if (https.begin(*client, _apiBase + endpoint)) {
        https.setTimeout(30000);
        applyBambuHeaders(https);
        https.addHeader("Authorization", "Bearer " + _authToken);
        
        httpCode = https.GET();
        if (httpCode == 200) {
            outPayload = https.getString();
        } else {
            Serial.printf("[CloudAPI] GET %s Failed. HTTP Code: %d\n", endpoint.c_str(), httpCode);
            Serial.printf("[CloudAPI] Server Response: %s\n", https.getString().c_str());
        }
        https.end();
    } else {
        httpCode = -1; 
    }
    
    delete client;
    return httpCode;
}

// ==========================================
// MODULE: UNIFIED 2-STEP AUTHENTICATION
// ==========================================
int BambuCloudAPI::login(String email, String password, String& outTfaKey) {
    email.trim(); 
    password.trim();
    
    Serial.println("\n[CloudAPI - AUTH] Step 1: Submitting Password...");

    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    client->setHandshakeTimeout(30000);
    HTTPClient https;

    if (https.begin(*client, _apiBase + "/v1/user-service/user/login")) {
        https.setTimeout(30000); 
        applyBambuHeaders(https);
        
        StaticJsonDocument<200> doc;
        doc["account"] = email;
        doc["password"] = password;
        doc["apiError"] = "";
        String requestBody;
        serializeJson(doc, requestBody);
        Serial.printf("[CloudAPI - AUTH] POST Body: %s\n", requestBody.c_str());

        int httpCode = https.POST(requestBody);
        
        if (httpCode == -1) {
            Serial.println("[CloudAPI - AUTH] Socket failed (-1). Retrying in 1s...");
            https.end(); delay(1000);
            if (https.begin(*client, _apiBase + "/v1/user-service/user/login")) {
                https.setTimeout(30000); applyBambuHeaders(https);
                httpCode = https.POST(requestBody);
            }
        }

        Serial.printf("[CloudAPI - AUTH] HTTP Response Code: %d\n", httpCode);
        String payload = https.getString();
        
        if (httpCode == 200) {
            DynamicJsonDocument respDoc(2048);
            deserializeJson(respDoc, payload);
            
            // 1. Direct Success (No 2FA required)
            if (respDoc.containsKey("success") && respDoc["success"].as<bool>()) {
                if (respDoc.containsKey("accessToken")) _authToken = respDoc["accessToken"].as<String>();
                else if (respDoc.containsKey("token")) _authToken = respDoc["token"].as<String>();
                
                config.cloudToken = _authToken;
                Serial.println("[CloudAPI - AUTH] Direct Login SUCCESS! Token saved.");
                https.end(); delete client; return 1;
            }
            
            // 2. 2FA Required
            if (respDoc.containsKey("loginType")) {
                String loginType = respDoc["loginType"].as<String>();
                
                if (loginType == "tfa") {
                    outTfaKey = respDoc["tfaKey"].as<String>();
                    Serial.println("[CloudAPI - AUTH] Authenticator App Code Required.");
                    https.end(); delete client; return 2;
                } 
                else if (loginType == "verifyCode") {
                    Serial.println("[CloudAPI - AUTH] Email Code Required. Triggering email...");
                    bool emailSent = requestEmailCode(email);
                    https.end(); delete client; return emailSent ? 3 : 0;
                }
            }
        } else {
            // RESTORED LOGGING
            Serial.println("[CloudAPI - AUTH] LOGIN FAILED.");
            Serial.printf("[CloudAPI - AUTH] Raw Server Payload: %s\n", payload.c_str());
            
            DynamicJsonDocument errDoc(1024);
            if (!deserializeJson(errDoc, payload)) {
                if (errDoc.containsKey("message")) {
                    Serial.printf("[CloudAPI - AUTH] -> Reason: %s\n", errDoc["message"].as<String>().c_str());
                } else if (errDoc.containsKey("error")) {
                    Serial.printf("[CloudAPI - AUTH] -> Reason: %s\n", errDoc["error"].as<String>().c_str());
                }
            }
        }
        https.end();
    }
    delete client;
    return 0;
}

bool BambuCloudAPI::submitTfaCode(String tfaKey, String code) {
    code.trim();
    Serial.println("\n[CloudAPI - AUTH] Step 2: Submitting Authenticator Code...");

    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    client->setHandshakeTimeout(30000);
    HTTPClient https;

    if (https.begin(*client, _apiBase + "/api/sign-in/tfa")) {
        https.setTimeout(30000);
        applyBambuHeaders(https);
        
        const char* headerKeys[] = {"Set-Cookie"};
        https.collectHeaders(headerKeys, 1);

        StaticJsonDocument<200> doc;
        doc["tfaKey"] = tfaKey;
        doc["tfaCode"] = code;
        String requestBody;
        serializeJson(doc, requestBody);

        int httpCode = https.POST(requestBody);
        Serial.printf("[CloudAPI - AUTH] HTTP Response Code: %d\n", httpCode);
        String payload = https.getString();

        if (httpCode == 200) {
            DynamicJsonDocument respDoc(1024);
            deserializeJson(respDoc, payload);

            String token = "";
            if (https.hasHeader("Set-Cookie")) {
                String cookie = https.header("Set-Cookie");
                int start = cookie.indexOf("token=");
                if (start != -1) {
                    start += 6;
                    int end = cookie.indexOf(";", start);
                    if (end == -1) end = cookie.length();
                    token = cookie.substring(start, end);
                }
            }

            if (token.length() == 0) {
                if (respDoc.containsKey("accessToken")) token = respDoc["accessToken"].as<String>();
                else if (respDoc.containsKey("token")) token = respDoc["token"].as<String>();
            }

            if (token.length() > 0) {
                _authToken = token;
                config.cloudToken = _authToken;
                Serial.println("[CloudAPI - AUTH] 2FA SUCCESS! Token saved.");
                https.end(); delete client; return true;
            }
        } else {
            // RESTORED LOGGING
            Serial.println("[CloudAPI - AUTH] 2FA CODE REJECTED.");
            Serial.printf("[CloudAPI - AUTH] Raw Server Payload: %s\n", payload.c_str());
            DynamicJsonDocument errDoc(1024);
            if (!deserializeJson(errDoc, payload)) {
                if (errDoc.containsKey("message")) {
                    Serial.printf("[CloudAPI - AUTH] -> Reason: %s\n", errDoc["message"].as<String>().c_str());
                }
            }
        }
        https.end();
    }
    delete client;
    return false;
}

bool BambuCloudAPI::requestEmailCode(String email) {
    email.trim(); 
    
    Serial.println("\n[CloudAPI - AUTH] Requesting verification code...");
    Serial.printf("[CloudAPI - AUTH] Cleaned Email: '%s'\n", email.c_str());

    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure(); 
    client->setHandshakeTimeout(30000);
    HTTPClient https;

    if (https.begin(*client, _apiBase + "/v1/user-service/user/sendemail/code")) {
        https.setTimeout(30000); 
        applyBambuHeaders(https);
        
        StaticJsonDocument<200> doc;
        doc["email"] = email;
        doc["type"] = "codeLogin"; 
        String requestBody;
        serializeJson(doc, requestBody);

        int httpCode = https.POST(requestBody);
        
        if (httpCode == -1) {
            Serial.println("[CloudAPI - AUTH] Socket failed (-1). Retrying in 1s...");
            https.end(); delay(1000);
            if (https.begin(*client, _apiBase + "/v1/user-service/user/sendemail/code")) {
                https.setTimeout(30000); applyBambuHeaders(https);
                httpCode = https.POST(requestBody);
            }
        }

        Serial.printf("[CloudAPI - AUTH] Final Response Code: %d\n", httpCode);
        
        bool success = (httpCode == 200 || httpCode == -5);
        if (!success) {
            // RESTORED LOGGING
            String payload = https.getString();
            Serial.println("[CloudAPI - AUTH] FAILED TO SEND EMAIL.");
            Serial.printf("[CloudAPI - AUTH] Raw Server Payload: %s\n", payload.c_str());
            
            DynamicJsonDocument errDoc(1024);
            if (!deserializeJson(errDoc, payload)) {
                if (errDoc.containsKey("message")) {
                    Serial.printf("[CloudAPI - AUTH] -> Reason: %s\n", errDoc["message"].as<String>().c_str());
                }
            }
        }
        
        https.end();
        delete client;
        return success;
    }
    delete client;
    return false;
}

bool BambuCloudAPI::loginWithCode(String email, String code) {
    email.trim();
    code.trim(); 
    
    Serial.println("\n[CloudAPI - AUTH] Attempting Login with Email Code...");

    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    client->setHandshakeTimeout(30000);
    HTTPClient https;

    if (https.begin(*client, _apiBase + "/v1/user-service/user/login")) {
        https.setTimeout(30000); 
        applyBambuHeaders(https);
        
        StaticJsonDocument<200> doc;
        doc["account"] = email;
        doc["code"] = code;
        String requestBody;
        serializeJson(doc, requestBody);

        int httpCode = https.POST(requestBody);
        
        if (httpCode == -1) {
            Serial.println("[CloudAPI - AUTH] Socket failed (-1). Retrying in 1s...");
            https.end(); delay(1000);
            if (https.begin(*client, _apiBase + "/v1/user-service/user/login")) {
                https.setTimeout(30000); applyBambuHeaders(https);
                httpCode = https.POST(requestBody);
            }
        }

        Serial.printf("[CloudAPI - AUTH] Final Response Code: %d\n", httpCode);
        String payload = https.getString();
        
        if (httpCode == 200) {
            DynamicJsonDocument respDoc(1024);
            deserializeJson(respDoc, payload);
            
            if (respDoc.containsKey("accessToken")) _authToken = respDoc["accessToken"].as<String>();
            else if (respDoc.containsKey("token")) _authToken = respDoc["token"].as<String>();
            
            config.cloudToken = _authToken;
            Serial.println("[CloudAPI - AUTH] Login SUCCESS! Token saved.");
            https.end(); delete client; return true;
        } else {
            // RESTORED LOGGING
            Serial.println("[CloudAPI - AUTH] EMAIL CODE REJECTED.");
            Serial.printf("[CloudAPI - AUTH] Raw Server Payload: %s\n", payload.c_str());
            DynamicJsonDocument errDoc(1024);
            if (!deserializeJson(errDoc, payload)) {
                if (errDoc.containsKey("message")) {
                    Serial.printf("[CloudAPI - AUTH] -> Reason: %s\n", errDoc["message"].as<String>().c_str());
                }
            }
        }
        https.end();
    }
    delete client;
    return false;
}

bool BambuCloudAPI::getUserInfo(String& outUid) {
    String payload;
    int httpCode = sendAuthenticatedGet("/v1/design-user-service/my/preference", payload);
    if (httpCode == 200) {
        DynamicJsonDocument respDoc(2048);
        deserializeJson(respDoc, payload);
        outUid = respDoc["uid"].as<String>();
        return true;
    }
    return false;
}

bool BambuCloudAPI::fetchPrinters(std::vector<PrinterInfo>& printers) {
    Serial.println("\n[CloudAPI - DEVICES] Fetching Bound Printers...");
    String payload;
    int httpCode = sendAuthenticatedGet("/v1/iot-service/api/user/bind", payload);
    
    if (httpCode == -1) {
        Serial.println("[CloudAPI - DEVICES] Socket failed (-1). Retrying in 1s...");
        delay(1000);
        httpCode = sendAuthenticatedGet("/v1/iot-service/api/user/bind", payload);
    }
    
    Serial.printf("[CloudAPI - DEVICES] Response Code: %d\n", httpCode);
    
    if (httpCode == 200) {
        DynamicJsonDocument respDoc(8192);
        deserializeJson(respDoc, payload);
        JsonArray devArray = respDoc["devices"].as<JsonArray>();
        
        for (JsonObject dev : devArray) {
            PrinterInfo p;
            p.name = dev["name"].as<String>();
            p.serial = dev["dev_id"].as<String>();
            
            p.ip = dev["ip"].isNull() ? "" : dev["ip"].as<String>(); 
            if (p.ip == "null") p.ip = "";
            
            p.accessCode = dev["dev_access_code"].isNull() ? "" : dev["dev_access_code"].as<String>();
            if (p.accessCode == "null") p.accessCode = "";
            
            printers.push_back(p);
            
            Serial.printf("[CloudAPI - DEVICES] Found Printer: %s (IP: %s)\n", p.name.c_str(), p.ip == "" ? "Unknown" : p.ip.c_str());
        }
        return true;
    }
    return false;
}