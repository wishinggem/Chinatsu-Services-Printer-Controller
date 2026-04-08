#include "BambuMQTT.h"
#include "BambuCloudAPI.h"

BambuMQTT* globalMQTTInstance = nullptr;
extern BambuCloudAPI cloudApi;

BambuMQTT::BambuMQTT() : _mqtt(_secureClient) {
    globalMQTTInstance = this;
    _lastPing = 0;
}

void BambuMQTT::begin(String serial, String userId, String cloudToken) {
    _serial = serial;
    _topicPub = "device/" + serial + "/request";
    _topicSub = "device/" + serial + "/report";

    Serial.println("\n[MQTT] Initializing Cloud MQTT connection...");
    _secureClient.setInsecure(); 
    
    static const char* alpnProtocols[] = { "mqtt", NULL };
    _secureClient.setAlpnProtocols(alpnProtocols);

    _mqtt.setServer("us.mqtt.bambulab.com", 8883); 
    _mqtt.setCallback(BambuMQTT::callback);
    
    _mqtt.setKeepAlive(60);
    _mqtt.setSocketTimeout(30);
    _mqtt.setBufferSize(8192); // Necessary for large payloads
    
    String clientId = "bambu-client-" + serial;
    
    String mqttUsername = userId;
    if (!mqttUsername.startsWith("u_")) {
        mqttUsername = "u_" + mqttUsername;
    }
    
    while (!_mqtt.connected()) {
        Serial.printf("[MQTT] Connecting to Cloud Broker as '%s'...\n", mqttUsername.c_str());
        if (_mqtt.connect(clientId.c_str(), mqttUsername.c_str(), cloudToken.c_str())) {
            Serial.println("[MQTT] SUCCESS!");
            _mqtt.subscribe(_topicSub.c_str());
            requestStatusUpdate();
        } else {
            int mqttState = _mqtt.state();
            Serial.printf("[MQTT] FAILED. PubSubClient State: %d\n", mqttState);
            
            if (mqttState == -4) Serial.println("[MQTT] -> Reason: Connection Timeout");
            else if (mqttState == -3) Serial.println("[MQTT] -> Reason: Connection Lost");
            else if (mqttState == -2) Serial.println("[MQTT] -> Reason: Connect Failed (Socket/Network Error)");
            else if (mqttState == 1) Serial.println("[MQTT] -> Reason: Bad Protocol");
            else if (mqttState == 2) Serial.println("[MQTT] -> Reason: Bad Client ID");
            else if (mqttState == 3) Serial.println("[MQTT] -> Reason: Unavailable");
            else if (mqttState == 4) Serial.println("[MQTT] -> Reason: Bad Credentials (Token expired?)");
            else if (mqttState == 5) Serial.println("[MQTT] -> Reason: Unauthorized");
            
            Serial.println("[MQTT] Retrying in 3s...");
            delay(3000);
        }
    }
}

void BambuMQTT::reconnect() {
    String clientId = "bambu-client-" + _serial;
    String mqttUsername = config.userId;
    if (!mqttUsername.startsWith("u_")) mqttUsername = "u_" + mqttUsername;
    
    Serial.printf("[MQTT] Attempting background reconnect as '%s'...\n", mqttUsername.c_str());
    if (_mqtt.connect(clientId.c_str(), mqttUsername.c_str(), config.cloudToken.c_str())) {
        Serial.println("[MQTT] Reconnected successfully!");
        _mqtt.subscribe(_topicSub.c_str());
        requestStatusUpdate();
    } else {
        Serial.printf("[MQTT] Background reconnect failed, state: %d\n", _mqtt.state());
    }
}

void BambuMQTT::loop() {
    if (!_mqtt.connected()) {
        static unsigned long lastMqttRetry = 0;
        if (millis() - lastMqttRetry > 5000) {
            lastMqttRetry = millis();
            reconnect();
        }
        return; // Skip the normal loop and pings if not connected
    }

    _mqtt.loop();
    if (millis() - _lastPing > 6000) {
        _lastPing = millis();
        requestStatusUpdate();
    }
}

void BambuMQTT::requestStatusUpdate() {
    StaticJsonDocument<200> doc;
    doc["pushing"]["sequence_id"] = "1";
    doc["pushing"]["command"] = "pushall";
     
    String payload;
    serializeJson(doc, payload);
    Serial.printf("[MQTT - TX] Sending Heartbeat: %s\n", payload.c_str());
    _mqtt.publish(_topicPub.c_str(), payload.c_str());
}

void BambuMQTT::sendAMSCommand(int amsId, int slotId, String material, String color) {
    DynamicJsonDocument doc(512);
    JsonObject print = doc.createNestedObject("print");
    print["command"] = "ams_change_filaments";
    print["sequence_id"] = "2";
    print["target"] = amsId;
    print["curr_temp"] = 0;
    print["tar_temp"] = 0;
    print["tray_color"] = color;
    print["tray_type"] = material;
    print["ams_id"] = amsId;
    print["tray_id"] = slotId;
    
    String payload;
    serializeJson(doc, payload);
    Serial.printf("[MQTT - TX] AMS Command: %s\n", payload.c_str());
    _mqtt.publish(_topicPub.c_str(), payload.c_str());
}

void BambuMQTT::sendGcode(String gcode) {
    DynamicJsonDocument doc(256);
    doc["print"]["sequence_id"] = "3";
    doc["print"]["command"] = "gcode_line";
    doc["print"]["param"] = gcode;
    
    String payload;
    serializeJson(doc, payload);
    Serial.printf("[MQTT - TX] Gcode Command: %s\n", payload.c_str());
    _mqtt.publish(_topicPub.c_str(), payload.c_str());
}

void BambuMQTT::pausePrint() {
    DynamicJsonDocument doc(256);
    doc["print"]["sequence_id"] = "4";
    doc["print"]["command"] = "pause";
    
    String payload;
    serializeJson(doc, payload);
    Serial.printf("[MQTT - TX] Pause Command: %s\n", payload.c_str());
    _mqtt.publish(_topicPub.c_str(), payload.c_str());
}

void BambuMQTT::resumePrint() {
    DynamicJsonDocument doc(256);
    doc["print"]["sequence_id"] = "5";
    doc["print"]["command"] = "resume";
    
    String payload;
    serializeJson(doc, payload);
    Serial.printf("[MQTT - TX] Resume Command: %s\n", payload.c_str());
    _mqtt.publish(_topicPub.c_str(), payload.c_str());
}

void BambuMQTT::stopPrint() {
    DynamicJsonDocument doc(256);
    doc["print"]["sequence_id"] = "6";
    doc["print"]["command"] = "stop";
    
    String payload;
    serializeJson(doc, payload);
    Serial.printf("[MQTT - TX] Stop Command: %s\n", payload.c_str());
    _mqtt.publish(_topicPub.c_str(), payload.c_str());
}

void BambuMQTT::setChamberLight(bool on) {
    DynamicJsonDocument doc(256);
    JsonObject sys = doc.createNestedObject("system");
    sys["sequence_id"] = "7";
    sys["command"] = "ledctrl";
    sys["led_node"] = "chamber_light";
    sys["led_mode"] = on ? "on" : "off";
    sys["led_on_time"] = 500;
    sys["led_off_time"] = 500;
    sys["loop_times"] = 0;
    sys["interval_time"] = 0;
    
    String payload;
    serializeJson(doc, payload);
    Serial.printf("[MQTT - TX] Chamber Light Command: %s\n", payload.c_str());
    _mqtt.publish(_topicPub.c_str(), payload.c_str());
}

void BambuMQTT::callback(char* topic, byte* payload, unsigned int length) {
    if (globalMQTTInstance) {
        globalMQTTInstance->parseStatusPayload(payload, length);
    }
}

// ==========================================
// THE MAGIC MEMORY-SAVING JSON PARSER
// ==========================================
void BambuMQTT::parseStatusPayload(byte* payload, unsigned int length) {
    // 1. Create a filter so we only parse the specific fields we care about.
    // This drops the memory requirement from ~20KB down to < 1KB!
    StaticJsonDocument<256> filter;
    filter["print"]["nozzle_temper"] = true;
    filter["print"]["nozzle_target_temper"] = true;
    filter["print"]["bed_temper"] = true;
    filter["print"]["bed_target_temper"] = true;
    filter["print"]["gcode_state"] = true;
    filter["print"]["mc_percent"] = true;
    filter["print"]["ams"] = true;

    // 2. Parse payload using the filter
    DynamicJsonDocument doc(4096); // Bumped up slightly to handle AMS arrays
    DeserializationError error = deserializeJson(doc, payload, length, DeserializationOption::Filter(filter));

    if (!error) {
        if (doc.containsKey("print")) {
            JsonObject print = doc["print"];
            
            // Update our global live data struct
            if (print.containsKey("nozzle_temper")) config.liveData.nozzleTemp = print["nozzle_temper"];
            if (print.containsKey("nozzle_target_temper")) config.liveData.nozzleTarget = print["nozzle_target_temper"];
            if (print.containsKey("bed_temper")) config.liveData.bedTemp = print["bed_temper"];
            if (print.containsKey("bed_target_temper")) config.liveData.bedTarget = print["bed_target_temper"];
            if (print.containsKey("mc_percent")) config.liveData.progress = print["mc_percent"];
            
            if (print.containsKey("gcode_state")) {
                String state = print["gcode_state"].as<String>();
                if (state == "IDLE") config.liveData.status = "Idle";
                else if (state == "RUNNING") config.liveData.status = "Printing";
                else if (state == "PAUSE") config.liveData.status = "Paused";
                else if (state == "PREPARE") config.liveData.status = "Preparing";
                else config.liveData.status = state;
            }
            
            // Capture AMS Slot Data
            if (print.containsKey("ams") && print["ams"].containsKey("ams")) {
                JsonArray amsList = print["ams"]["ams"];
                if (amsList.size() > 0) {
                    if (amsList[0].containsKey("humidity")) {
                        config.liveData.amsHumidity = amsList[0]["humidity"].as<String>();
                    }
                    JsonArray trays = amsList[0]["tray"];
                    for (int i = 0; i < 4; i++) {
                        config.liveData.ams[i].type = "";
                        config.liveData.ams[i].color = "";
                    }
                    for (JsonObject tray : trays) {
                        if (tray.containsKey("id")) {
                            int id = tray["id"].as<String>().toInt();
                            if (id >= 0 && id < 4) {
                                if (tray.containsKey("tray_color")) config.liveData.ams[id].color = tray["tray_color"].as<String>();
                                if (tray.containsKey("tray_type")) config.liveData.ams[id].type = tray["tray_type"].as<String>();
                            }
                        }
                    }
                }
            }
        }
    } else {
        Serial.print("[MQTT] JSON Parse Error: ");
        Serial.println(error.c_str());
    }
}