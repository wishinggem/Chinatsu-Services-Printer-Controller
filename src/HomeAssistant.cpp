#include "HomeAssistant.h"
#include "Config.h"
#include "BambuMQTT.h"
#include "pages/PageMain.h"
#include <HTTPClient.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

extern BambuMQTT printerMqtt;

HomeAssistant::HomeAssistant() {
    _lastSendTime = 0;
}

void HomeAssistant::begin(AsyncWebServer* server) {
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/command", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        if (json.is<JsonObject>()) {
            JsonObject jsonObj = json.as<JsonObject>();
            if (jsonObj.containsKey("action")) {
                Serial.println("\n========== API Action Command Received ==========");
                String action = jsonObj["action"].as<String>();
                if (action == "toggle_light") {
                    Serial.println("Received Brighness Toggle Light Command");  
                    chamberLightOn = !chamberLightOn;
                    printerMqtt.setChamberLight(chamberLightOn);
                    request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Light toggled\"}");
                    return;
                } else if (action == "force_sync" || action == "force_send") {
                    Serial.println("Received Sync Command");  
                    this->sendData();
                    request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Data synced\"}");
                    return;
                } else if (action == "pause_print") {
                    Serial.println("Received Brighness Print Pause Command");  
                    printerMqtt.pausePrint();
                    request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Print paused\"}");
                    return;
                } else if (action == "resume_print") {
                    Serial.println("Received Brighness Print Resume Command");  
                    printerMqtt.resumePrint();
                    request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Print resumed\"}");
                    return;
                } else if (action == "stop_print") {
                    Serial.println("Received Brighness Print Stop Command");  
                    printerMqtt.stopPrint();
                    request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Print stopped\"}");
                    return;
                } else if (action == "set_brightness") {
                    if (jsonObj.containsKey("value")) {
                        Serial.println("Received Brighness Update Command: " + String(jsonObj["value"].as<int>()));                
                        config.brightness = jsonObj["value"].as<int>();
                        saveSettings();
                        config.lastTouchTime = millis(); // Wake screen immediately to apply
                        request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Brightness updated\"}");
                        return;
                    }
                } else if (action == "set_timeout") {
                    if (jsonObj.containsKey("value")) {
                        Serial.println("Received Timeout Update Command: " + String(jsonObj["value"].as<int>()));                
                        config.screenTimeout = jsonObj["value"].as<int>();
                        saveSettings();
                        request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Timeout updated\"}");
                        return;
                    }
                }
                Serial.println("===================================================\n");
            }
        }
        request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid command payload\"}");
    });
    server->addHandler(handler);
}

void HomeAssistant::loop() {
    if (!config.haSendEnabled) return;
    if (config.haIp.length() == 0 || config.haEndpoint.length() == 0) return;
    
    if (config.haSendInterval > 0) {
        if (millis() - _lastSendTime > (config.haSendInterval * 60000UL)) {
            _lastSendTime = millis();
            sendData();
        }
    }
}

String HomeAssistant::buildPayload() {
    DynamicJsonDocument doc(8192);
    
    JsonObject account = doc.createNestedObject("account_info");
    account["username"] = config.userEmail;

    JsonObject printer = doc.createNestedObject("printer_info");
    printer["name"] = config.activePrinter.name;
    printer["light_status"] = chamberLightOn ? "ON" : "OFF";
    printer["wifi_ssid"] = config.wifiSSID;

    JsonObject device = doc.createNestedObject("device_info");
    device["screen_brightness"] = config.brightness;
    device["screen_timeout"] = config.screenTimeout;

    JsonObject print = doc.createNestedObject("printing_info");
    print["layer"] = printLayer;
    print["total_layers"] = printTotalLayers;
    print["remaining_mins"] = printRemainingTime;
    print["speed"] = printSpeedLevel;
    print["nozzle_temp"] = config.liveData.nozzleTemp;
    print["nozzle_target"] = config.liveData.nozzleTarget;
    print["bed_temp"] = config.liveData.bedTemp;
    print["bed_target"] = config.liveData.bedTarget;
    print["fan_part"] = fanPartSpeed;
    print["fan_aux"] = fanAuxSpeed;
    print["fan_chamber"] = fanChamberSpeed;
    print["gcode_file"] = printGcodeFile;
    print["status"] = config.liveData.status;
    print["progress"] = config.liveData.progress;

    JsonObject ams = doc.createNestedObject("ams_info");
    ams["humidity"] = config.liveData.amsHumidity;
    ams["dry_time"] = config.liveData.amsDryTime;
    ams["dry_temp"] = config.liveData.amsDryTemp;
    ams["active_tray"] = amsActiveTray;

    JsonArray trays = ams.createNestedArray("trays");
    for (int i = 0; i < 4; i++) {
        JsonObject tray = trays.createNestedObject();
        tray["id"] = i;
        bool empty = (config.liveData.ams[i].type == "" || config.liveData.ams[i].color == "");
        tray["empty"] = empty;
        tray["color"] = config.liveData.ams[i].color;
        tray["type"] = config.liveData.ams[i].type;
        tray["remain"] = amsRemain[i];
        tray["brand"] = amsBrand[i];
        
        const uint8_t* selectedLogo = logo_generic;
        String brandUpper = amsBrand[i];
        brandUpper.toUpperCase();
        for (int k = 0; k < sizeof(KNOWN_LOGOS)/sizeof(KNOWN_LOGOS[0]); k++) {
            String dictBrand = KNOWN_LOGOS[k].brand;
            dictBrand.toUpperCase();
            if (brandUpper.indexOf(dictBrand) >= 0) {
                selectedLogo = KNOWN_LOGOS[k].bitmap;
                break;
            }
        }
        
        JsonArray icon_bytes = tray.createNestedArray("icon_bytes");
        for(int b=0; b<32; b++) icon_bytes.add(selectedLogo[b]);
    }

    JsonArray hms = doc.createNestedArray("hms");
    for (const auto& err : hmsHistory) {
        JsonObject hms_err = hms.createNestedObject();
        hms_err["code"] = err.code;
        HMSDictEntry entry = getHMSEntry(err.code);
        hms_err["description"] = entry.shortDesc;
        hms_err["long_description"] = entry.longDesc;
    }

    String parsedPayload;
    serializeJson(doc, parsedPayload);

    String payload = "{\"parsed_data\":";
    payload += parsedPayload;
    payload += ",\"mqtt_data\":";
    payload += lastRawMqttMessage.length() > 0 ? lastRawMqttMessage : "{}";
    payload += "}";

    return payload;
}

void HomeAssistant::dumpPayload() {
    String payload = buildPayload();
    Serial.println("\n========== HOME ASSISTANT PAYLOAD DUMP ==========");
    Serial.println(payload);
    Serial.println("===================================================\n");
}

void HomeAssistant::sendData() {
    if (config.haIp.length() == 0 || config.haEndpoint.length() == 0) return;

    String payload = buildPayload();

    HTTPClient http;
    String endpoint = config.haEndpoint;
    if (!endpoint.startsWith("/")) endpoint = "/" + endpoint; // Safely enforce preceding slash
    
    String url = "http://" + config.haIp + ":" + String(config.haPort) + endpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) {
        Serial.printf("[HomeAssistant] Data sent successfully. Code: %d\n", httpResponseCode);
    } else {
        Serial.printf("[HomeAssistant] Error sending data. Code: %d\n", httpResponseCode);
    }
    http.end();
}