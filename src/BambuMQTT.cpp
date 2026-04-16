#include "BambuMQTT.h"
#include "BambuCloudAPI.h"
#include "pages/PageManager.h"

#ifndef STATE_PRINT_MONITOR
#define STATE_PRINT_MONITOR 11
#endif

BambuMQTT* globalMQTTInstance = nullptr;
extern BambuCloudAPI cloudApi;
extern PageManager uiManager;

std::vector<HMSError> hmsHistory;
bool hasNewHmsError = false;
bool chamberLightOn = false;

int printLayer = 0;
int printTotalLayers = 0;
int printRemainingTime = 0;
int printSpeedLevel = 2;
int fanPartSpeed = 0;
int fanAuxSpeed = 0;
int fanChamberSpeed = 0;
String printGcodeFile = "";
bool hasAutoSwitchedToPrint = false;
int amsRemain[4] = {-1, -1, -1, -1};
String amsBrand[4] = {"", "", "", ""};
int amsActiveTray = -1;
bool dumpNextMqttPacket = false;

HMSDictEntry getHMSEntry(String code) {
    String c = code;
    c.replace("-", "");
    
    if (c == "0300800A") return {"Filament pile-up", "A Filament pile-up was detected by the AI Print Monitoring. Please clean the filament from the waste chute."};
    if (c == "03008003") return {"Spaghetti detected", "Spaghetti defects were detected by the AI Print Monitoring. Please check the quality of the printed model before continuing your print."};
    if (c == "07008010" || c == "07018010" || c == "07FF8010" || c == "07028010" || c == "07038010") return {"AMS motor overloaded", "AMS assist motor is overloaded. Please check if the spool or filament is stuck."};
    if (c == "03004008") return {"AMS change failed", "The AMS failed to change filament."};
    if (c == "07008011" || c == "07018011" || c == "07028011" || c == "07038011" || c == "12008011") return {"AMS filament ran out", "AMS filament ran out. Please insert a new filament into the same AMS slot."};
    if (c == "05004016") return {"SD Card write-protected", "The MicroSD Card is write-protected. Please replace the MicroSD Card."};
    if (c == "0500402F") return {"SD sector damaged", "The Micro SD card sector data is damaged. Please use the SD card repair tool to repair or format it."};
    if (c == "0500C010") return {"SD read/write exception", "MicroSD Card read/write exception. please reinsert or replace MicroSD Card."};
    if (c == "03004009") return {"Homing XY failed", "Homing XY axis failed."};
    if (c == "03004000") return {"Homing Z failed", "Printing stopped because homing Z axis failed."};
    if (c == "05004005") return {"Updating firmware", "Print jobs are not allowed to be sent while updating firmware."};
    if (c == "05004009") return {"Updating logs", "Print jobs are not allowed to be sent while updating logs."};
    if (c == "05004002") return {"Unsupported file", "Unsupported print file path or name. Please resend the printing job."};
    if (c == "05014032") return {"QR binding active", "QR code binding is in progress, so device discovery binding cannot be performed."};
    if (c == "05004014") return {"Slicing failed", "Slicing for the print job failed. Please check your settings and restart the print job."};
    if (c == "0500401F" || c == "0501401F") return {"Auth timed out", "Authorization timed out. Please make sure that your phone or PC has access to the internet."};
    if (c == "0500401D" || c == "0501401D" || c == "05004021" || c == "05004027" || c == "05014021" || c == "05014027" || c == "05004024" || c == "05014024" || c == "0500401A" || c == "0501401A") return {"Cloud access failed", "Cloud access failed. Possible reasons include network instability caused by interference."};
    if (c == "0500401C" || c == "0501401C" || c == "05004020" || c == "05004023" || c == "05004026" || c == "05004029" || c == "05014020" || c == "05014023" || c == "05014026" || c == "05014029") return {"Cloud access rejected", "Cloud access is rejected. If you have tried multiple times and are still failing, please contact customer service."};
    if (c == "0500401B" || c == "0500401E" || c == "0501401B" || c == "0501401E" || c == "05004022" || c == "05004025" || c == "05004028" || c == "05014022" || c == "05014025" || c == "05014028") return {"Cloud response invalid", "Cloud response is invalid. If you have tried multiple times and are still failing, please contact customer service."};
    if (c == "05004006") return {"Storage full", "There is not enough free storage space for the print job."};
    if (c == "0300400B") return {"Comm exception", "Internal communication exception."};
    if (c == "0300800B") return {"Cutter stuck", "The cutter is stuck. Please make sure the cutter handle is out."};
    if (c == "12008001") return {"Cut failed", "Cutting the filament failed. Please check to see if the cutter is stuck."};
    if (c == "1001C001") return {"Timelapse unsupported", "Timelapse is not supported because Spiral vase is enabled in slicing presets."};
    if (c == "1001C002") return {"Timelapse unsupported", "Timelapse is not supported because Print sequence is set to \"By object\"."};
    if (c == "05004038") return {"Nozzle mismatch", "The nozzle diameter in sliced file is not consistent with the current nozzle setting."};
    if (c == "05008036") return {"Printer mismatch", "Your sliced file is not consistent with the current printer model. Continue?"};
    if (c == "05004037") return {"Printer incompatible", "Your sliced file is not compatible with current printer model. This file can't be printed."};
    if (c == "05014034") return {"Slicing timeout", "The slicing progress has not been updated for a long time, and the printing task has exited."};
    if (c == "05014038") return {"Region mismatch", "The regional settings do not match the printer, please check the printer's regional settings."};
    if (c == "05004008") return {"Start print failed", "Starting printing failed. please power cycle the printer and resend the print job."};
    if (c == "03004006") return {"Nozzle clogged", "The nozzle is clogged."};
    if (c == "03008008") return {"Nozzle temp err", "Printing Stopped because nozzle temperature problem."};
    if (c == "03004005") return {"Nozzle fan err", "The nozzle fan speed is abnormal."};
    if (c == "07FF8012" || c == "12FF8012" || c == "07008012" || c == "07018012" || c == "07028012" || c == "07038012" || c == "12008012" || c == "12018012" || c == "12028012" || c == "12038012") return {"AMS mapping failed", "Failed to get AMS mapping table; please click \"Retry\" to continue."};
    if (c == "12018014" || c == "12028014" || c == "12038014") return {"Filament location check failed", "Failed to check the filament location in the tool head, please refer to the HMS."};
    if (c == "03008005") return {"Front cover fell off", "Toolhead front cover fell off. Please remount the front cover and check to make sure your print is going okay."};
    if (c == "03004001") return {"Cooldown timeout", "The printer timed out waiting for the nozzle to cool down before homing."};
    if (c == "1000C003") return {"Timelapse warning", "Enabling traditional timelapse might lead to defects."};
    if (c == "0500403A") return {"Temp too low", "The current temperature is too low. In order to protect you and your printer. Printing task is disabled."};
    if (c == "05014033") return {"APP region mismatch", "Your APP region is not matched with your printer, please download the APP in the corresponding region."};
    if (c == "0300400C" || c == "0500400E") return {"Print cancelled", "Printing was cancelled."};
    if (c == "05004019" || c == "05014019") return {"Already bound", "The printer has already been bound. Please unbind it and try again."};
    if (c == "03008004" || c == "03008015") return {"Filament ran out", "Filament ran out. Please load new filament."};
    if (c == "05004004") return {"Printer busy", "The printer can't receive new print jobs while printing. Resend after the current print finishes."};
    if (c == "0300400A") return {"Resonance failed", "Mechanical resonance frequency identification failed."};
    if (c == "0500400B") return {"Download failed", "There was a problem downloading a file. Please check you network connection."};
    if (c == "0500400A") return {"Invalid filename", "The file name is not supported. Please rename and restart the printing job."};
    if (c == "07FF8004" || c == "07028004" || c == "07038004" || c == "12FF8004" || c == "12008004" || c == "12018004" || c == "12028004" || c == "12038004" || c == "12018015" || c == "12028015" || c == "12038015" || c == "07008004" || c == "07018004") return {"Pull back failed", "Failed to pull back the filament from the toolhead to AMS. Please check whether the filament or the spool is stuck."};
    if (c == "07028007" || c == "07038007" || c == "07018007" || c == "12018007" || c == "12028007" || c == "12038007" || c == "07008007" || c == "12008007") return {"Extrude failed", "Failed to extrude the filament. Please check if the extruder clogged."};
    if (c == "12008003" || c == "12018003" || c == "12028003" || c == "12038003" || c == "07028003" || c == "07038003" || c == "07008003" || c == "07018003") return {"Pull out failed", "Failed to pull out the filament from the extruder. Please check whether the extruder is clogged."};
    if (c == "07FF8005" || c == "07028005" || c == "07038005" || c == "07008005" || c == "07018005") return {"AMS feed failed", "Failed to feed the filament outside the AMS. Please clip the end of the filament flat and check to see if the spool is stuck."};
    if (c == "07028006" || c == "07038006" || c == "12008006" || c == "12018006" || c == "12028006" || c == "12038006" || c == "07008006" || c == "07018006") return {"Toolhead feed failed", "Failed to feed the filament into the toolhead. Please check whether the filament or the spool is stuck."};
    if (c == "0300400D") return {"Resume failed", "Resume failed after power loss."};
    if (c == "12008014") return {"Filament location unknown", "The filament location in the toolhead was not found. Refer to the Assistant for solutions."};
    if (c == "05004001") return {"Cloud disconnected", "Failed to connect to Bambu Cloud. Please check your network connection."};
    if (c == "0500402C") return {"DHCP IP failed", "Failed to obtain IP address, which may be caused by wireless interference resulting in data transmission failure."};
    if (c == "0C008009") return {"Marker not found", "Build plate localization marker was not found."};
    if (c == "03008006") return {"Marker not detected", "The build plate marker was not detected. Please confirm the build plate is correctly positioned on the heatbed."};
    if (c == "03008000") return {"Print paused", "Printing was paused for unknown reason. You can tap \"Resume\" to resume the print job."};
    if (c == "07FF8011" || c == "12FF8011" || c == "12018011" || c == "12028011" || c == "12038011") return {"AMS empty", "AMS filament ran out. Please put a new filament into AMS and click the \"Retry\" button."};
    if (c == "0300800C") return {"Layer shift detect", "Skipping step detected, auto-recover complete; please check if there are any layer shift problems."};
    if (c == "12FF8013" || c == "12008013" || c == "12018013" || c == "12028013" || c == "12038013" || c == "07FF8013" || c == "07008013" || c == "07018013" || c == "07028013" || c == "07038013") return {"Purge timeout", "Timeout while purging old filament. Please check if the filament is stuck or the extruder clogged."};
    if (c == "07FF8002" || c == "12FF8002" || c == "07008002" || c == "07018002" || c == "07028002" || c == "07038002" || c == "12008002" || c == "12018002" || c == "12028002" || c == "12038002") return {"Cutter stuck", "The cutter is stuck. Please pull out the cutter handle and click the \"Retry\" button."};
    if (c == "07FF8001" || c == "12FF8001" || c == "07008001" || c == "07018001" || c == "07028001" || c == "07038001" || c == "12018001" || c == "12028001" || c == "12038001") return {"Cut failed", "Failed to cut the filament. Please check the cutter."};
    if (c == "0C00C004" || c == "0C008002") return {"Spaghetti detected", "Possible spaghetti failure was detected."};
    if (c == "0C00C006" || c == "0C008005") return {"Chute pile up", "Purged filament may have piled up in the waste chute."};
    if (c == "0300800E" || c == "05008013") return {"File missing", "The print file is not available. Please check to see if the storage media has been removed."};
    if (c == "12008016" || c == "12018016" || c == "12028016" || c == "12038016") return {"Extruder issue", "The extruder is not extruding normally, please refer to the HMS."};
    if (c == "03008016") return {"Nozzle clogged", "The nozzle is clogged up with filaments. Please cancel this printing and clean the nozzle."};
    if (c == "03008014") return {"Nozzle covered", "The nozzle is covered with filaments, or the build plate is installed incorrectly."};
    if (c == "03008007") return {"Unfinished print", "There was an unfinished print job when the printer lost power."};
    if (c == "0300800D") return {"Objects fallen", "Some objects have fallen down, or the extruder is not extruding normally."};
    if (c == "03008017") return {"Foreign objects", "Foreign objects detected on hotbed, Please check and clean the hotbed."};
    if (c == "03008011" || c == "0C00800A") return {"Wrong build plate", "Detected build plate is not the same as the Gcode file."};
    if (c == "12FF4001" || c == "07004001" || c == "07014001" || c == "07024001" || c == "07034001" || c == "12004001" || c == "12014001" || c == "12024001" || c == "12034001" || c == "07FF4001") return {"AMS filament loaded", "Filament is still loaded from the AMS when it has been disabled."};
    if (c == "0300800F") return {"Door open", "The door seems to be open, so printing was paused."};
    if (c == "0C00C003" || c == "0C008001" || c == "03008002") return {"First layer defect", "Possible defects were detected in the first layer."};
    if (c == "03008019") return {"No build plate", "No build plate is placed."};
    if (c == "1000C001") return {"Bed temp high", "High bed temperature may lead to filament clogging in the nozzle."};
    if (c == "03008009") return {"Bed temp error", "Heatbed temperature malfunction."};
    if (c == "03004002") return {"ABL failed", "Printing Stopped because Auto Bed Leveling failed."};
    if (c == "03008010") return {"Hotend fan error", "Printing stopped because the hotend fan speed is abnormal."};
    if (c == "1000C002") return {"CF SS warning", "Printing CF material with stainless steel may cause nozzle damage."};
    if (c == "03008001" || c == "03008013") return {"User paused", "Printing was paused by the user."};
    if (c == "0300400E") return {"Motor check failed", "The motor self-check failed."};
    if (c == "0500402E") return {"SD format error", "The system does not support the file system currently used by the Micro SD card."};
    if (c == "0500402D") return {"System exception", "System exception."};
    if (c == "05004017" || c == "05014017") return {"Binding failed", "Binding failed. Please retry or restart the printer and retry."};
    if (c == "05004018" || c == "05014018") return {"Binding config err", "Binding configuration information parsing failed, please try again."};
    if (c == "12008010" || c == "12018010" || c == "12028010" || c == "12038010" || c == "12FF8010") return {"Spool stuck", "Filament or spool may be stuck."};
    if (c == "12008015") return {"Toolhead pull fail", "Failed to pull out the filament from the toolhead. Please check if the filament is stuck."};
    if (c == "03008018") return {"Chamber temp err", "Chamber temperature malfunction."};
    if (c == "05004003") return {"Parse failed", "Printing stopped because the printer was unable to parse the file. Please resend your print job."};
    if (c == "05014031") return {"Binding active", "Device discovery binding is in progress, and the QR code cannot be displayed on the screen."};
    if (c == "05004015") return {"Storage full", "There is not enough free storage space for the print job. Please format or clean MicroSD card."};
    if (c == "05014035") return {"Binding busy", "The device is in the process of binding and cannot respond to new binding requests."};
    if (c == "05144039" || c == "0514039") return {"Login expired", "Device login has expired, please try to bind again."};
    if (c == "07FF8003" || c == "07FFC003" || c == "12FF8003" || c == "12FFC003") return {"Spool holder pull", "Please pull out the filament on the spool holder."};
    if (c == "07FF8006" || c == "07FFC006" || c == "12FF8006" || c == "12FFC006") return {"Feed PTFE", "Please feed filament into the PTFE tube until it can not be pushed any farther."};
    if (c == "0500400C") return {"Insert SD", "Please insert a MicroSD card and restart the printing job."};
    if (c == "12FF8005" || c == "12008005" || c == "12018005" || c == "12028005" || c == "12038005") return {"Feed failed", "Failed to feed the filament. Please load the filament, then click the \"Retry\" button."};
    if (c == "07FF8007" || c == "12FF8007") return {"Observe nozzle", "Please observe the nozzle. If the filament has been extruded, click \"Done\"; if it is not, please push the filament forward slightly."};
    if (c == "0500400D") return {"Self-test required", "Please run a self-test and restart the printing job."};
    if (c == "0500402A") return {"WiFi weak", "Failed to connect to the router, which may be caused by wireless interference or being too far away from the router."};
    if (c == "0500402B") return {"WiFi password", "Router connection failed due to incorrect password. Please check the password and try again."};
    if (c == "05004007") return {"Update required", "Print jobs are not allowed to be sent while force updating or when repair updating is required."};
    if (c == "05000500") return {"No MQTT Auth", "MQTT Command verification failed, please update Studio or Handy."};
    
    return {"Unknown Error", "An unknown error occurred. Code: " + code};
}

String getFilamentBrand(String idx, String subBrand) {
    // --- BAMBU LAB OFFICIAL (GF series) ---
    if (idx == "GFA00") return "Bambu"; // Bambu PLA Basic
    if (idx == "GFA01") return "Bambu"; // Bambu PLA Matte
    if (idx == "GFA02") return "Bambu"; // Bambu PLA Sparkle
    if (idx == "GFA03") return "Bambu"; // Bambu PLA Tough
    if (idx == "GFA04") return "Bambu"; // Bambu PLA Galaxy
    if (idx == "GFA05") return "Bambu"; // Bambu PLA-CF
    if (idx == "GFA07") return "Bambu"; // Bambu PLA Silk
    if (idx == "GFA08") return "Bambu"; // Bambu Support for PLA
    if (idx == "GFA09") return "Bambu"; // Bambu PLA Aero
    if (idx == "GFA11") return "Bambu"; // Bambu PLA Metal
    if (idx == "GFA12") return "Bambu"; // Bambu PLA Marble
    if (idx == "GFA13") return "Bambu"; // Bambu PLA Glow
    if (idx == "GFA14") return "Bambu"; // Bambu PLA Silk Dual Color
    if (idx == "GFG00") return "Bambu"; // Bambu PETG Basic
    if (idx == "GFG01") return "Bambu"; // Bambu PETG-CF
    if (idx == "GFG02") return "Bambu"; // Bambu PETG HF
    if (idx == "GFG03") return "Bambu"; // Bambu PETG Translucent
    if (idx == "GFG50") return "Bambu"; // Bambu Support for PETG
    if (idx == "GFN00") return "Bambu"; // Bambu ABS
    if (idx == "GFN01") return "Bambu"; // Bambu ABS-GF
    if (idx == "GFN02") return "Bambu"; // Bambu ABS-CF
    if (idx == "GFC00") return "Bambu"; // Bambu PA-CF
    if (idx == "GFC01") return "Bambu"; // Bambu PAHT-CF
    if (idx == "GFC02") return "Bambu"; // Bambu PA6-CF
    if (idx == "GFC03") return "Bambu"; // Bambu PA6-GF
    if (idx == "GFS00") return "Bambu"; // Bambu ASA
    if (idx == "GFS01") return "Bambu"; // Bambu ASA-Aero
    if (idx == "GFU01") return "Bambu"; // Bambu TPU 95A
    if (idx == "GFU02") return "Bambu"; // Bambu TPU for AMS
    if (idx == "GFU03") return "Bambu"; // Bambu TPU 95A HF
    if (idx == "GFB00") return "Bambu"; // Bambu PC
    if (idx == "GFB01") return "Bambu"; // Bambu PC-CF
    
    // --- THIRD PARTY PRESETS (GFL series) ---
    if (idx == "GFL99") return "Generic"; // Generic PLA
    if (idx == "GFL98") return "Generic"; // Generic PETG
    if (idx == "GFL97") return "Generic"; // Generic ABS
    if (idx == "GFL96") return "Generic"; // Generic ASA
    if (idx == "GFL95") return "Generic"; // Generic TPU
    if (idx == "GFL94") return "Generic"; // Generic PC
    if (idx == "GFL93") return "Generic"; // Generic PA-CF
    if (idx == "GFL92") return "Generic"; // Generic PA
    if (idx == "GFL91") return "Generic"; // Generic PET
    if (idx == "GFL90") return "Generic"; // Generic PET-CF
    if (idx == "GFL89") return "Generic"; // Generic PVA
    if (idx == "GFL88") return "Generic"; // Generic HIPS
    if (idx == "GFL00") return "Sunlu"; // PolyTerra / Sunlu PLA
    if (idx == "GFL01") return "eSUN"; // eSUN PLA+
    if (idx == "GFL02") return "Hatchbox"; // Hatchbox PLA
    if (idx == "GFL03") return "PolyLite"; // PolyLite PLA
    if (idx == "GFL04") return "PolyLite"; // PolyLite PETG
    if (idx == "GFL05") return "Overture"; // Overture PLA
    if (idx == "GFL06") return "Overture"; // Overture PETG
    if (idx == "GFL07") return "eSUN"; // eSUN PETG
    
    // --- SPECIAL / SUPPORT ---
    if (idx == "GFS99") return "Generic"; // Generic Support for PLA
    if (idx == "GFS98") return "Generic"; // Generic Support for PA/PET

    // --- UNKNOWN / UNRECOGNIZED / GENERIC ---
    if (idx == "GFG99") return "Generic"; // Generic PETG
    
    // Edge cases / Fallbacks
    if (idx.startsWith("GF") && !idx.startsWith("GFL") && !idx.startsWith("GFS")) return "Bambu";
    if (idx.startsWith("GFL") || idx.startsWith("GFS")) return "Generic";

    if (subBrand != "") {
        String subLower = subBrand;
        subLower.toLowerCase();
        if (subLower.indexOf("bambu") >= 0) return "Bambu";
    }
    return "Generic";
}

BambuMQTT::BambuMQTT() : _mqtt(_secureClient) {
    globalMQTTInstance = this;
    _lastUpdateRx = 0;
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
    _mqtt.setBufferSize(16384); // Necessary for large payloads during printing
    
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
    if (millis() - _lastUpdateRx > 300000) { // Force update if no message received in 5 mins
        _lastUpdateRx = millis();
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
    static int lightSeq = 7;
    DynamicJsonDocument doc(256);
    JsonObject sys = doc.createNestedObject("system");
    sys["sequence_id"] = String(lightSeq++);
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
        if (dumpNextMqttPacket) {
            // A full "pushall" payload is typically >2000 bytes, while normal differential updates are ~150 bytes.
            if (length > 1000) {
                Serial.println("\n========== FULL MQTT PAYLOAD DUMP ==========");
                for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
                Serial.println("\n============================================\n");
                dumpNextMqttPacket = false;
            }
        }
        
        globalMQTTInstance->_lastUpdateRx = millis();
        globalMQTTInstance->parseStatusPayload(payload, length);
    }
}

// ==========================================
// THE MAGIC MEMORY-SAVING JSON PARSER
// ==========================================
void BambuMQTT::parseStatusPayload(byte* payload, unsigned int length) {
    // 1. Create a filter so we only parse the specific fields we care about.
    // This drops the memory requirement from ~20KB down to < 1KB!
    StaticJsonDocument<512> filter;
    filter["print"]["nozzle_temper"] = true;
    filter["print"]["nozzle_target_temper"] = true;
    filter["print"]["bed_temper"] = true;
    filter["print"]["bed_target_temper"] = true;
    filter["print"]["gcode_state"] = true;
    filter["print"]["mc_percent"] = true;
    filter["print"]["layer_num"] = true;
    filter["print"]["total_layer_num"] = true;
    filter["print"]["mc_remaining_time"] = true;
    filter["print"]["spd_lvl"] = true;
    filter["print"]["cooling_fan_speed"] = true;
    filter["print"]["big_fan1_speed"] = true;
    filter["print"]["big_fan2_speed"] = true;
    filter["print"]["gcode_file"] = true;
    filter["print"]["ams"] = true;
    filter["print"]["hms"] = true;
    filter["print"]["lights_report"] = true;

    // 2. Parse payload using the filter
    DynamicJsonDocument doc(8192); // Bumped up to safely handle deeply nested/large filtered structures
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
            
            if (print.containsKey("layer_num")) printLayer = print["layer_num"];
            if (print.containsKey("total_layer_num")) printTotalLayers = print["total_layer_num"];
            if (print.containsKey("mc_remaining_time")) printRemainingTime = print["mc_remaining_time"];
            if (print.containsKey("spd_lvl")) printSpeedLevel = print["spd_lvl"];
            if (print.containsKey("cooling_fan_speed")) fanPartSpeed = print["cooling_fan_speed"].as<String>().toInt() * 100 / 15;
            if (print.containsKey("big_fan1_speed")) fanAuxSpeed = print["big_fan1_speed"].as<String>().toInt() * 100 / 15;
            if (print.containsKey("big_fan2_speed")) fanChamberSpeed = print["big_fan2_speed"].as<String>().toInt() * 100 / 15;
            if (print.containsKey("gcode_file")) printGcodeFile = print["gcode_file"].as<String>();
            
            if (print.containsKey("gcode_state")) {
                String state = print["gcode_state"].as<String>();
                if (state == "IDLE") config.liveData.status = "Idle";
                else if (state == "RUNNING") config.liveData.status = "Printing";
                else if (state == "PAUSE") config.liveData.status = "Paused";
                else if (state == "PREPARE") config.liveData.status = "Preparing";
                else config.liveData.status = state;
                
                if (state == "RUNNING" || state == "PREPARE") {
                    if (!hasAutoSwitchedToPrint) {
                        hasAutoSwitchedToPrint = true;
                        if (config.currentState != static_cast<AppState>(STATE_PRINT_MONITOR)) uiManager.switchPage(static_cast<AppState>(STATE_PRINT_MONITOR));
                    }
                } else if (state == "IDLE" || state == "FINISH" || state == "FAILED") {
                    hasAutoSwitchedToPrint = false;
                }
            }
            
            // Capture AMS Slot Data
            if (print.containsKey("ams")) {
                if (print["ams"].containsKey("tray_now")) {
                    amsActiveTray = print["ams"]["tray_now"].as<String>().toInt();
                }
                if (print["ams"].containsKey("ams")) {
                    JsonArray amsList = print["ams"]["ams"];
                    if (amsList.size() > 0) {
                    if (amsList[0].containsKey("humidity")) {
                        config.liveData.amsHumidity = amsList[0]["humidity"].as<String>();
                    }
                    if (amsList[0].containsKey("humidity_raw")) {
                        config.liveData.amsHumidity = amsList[0]["humidity_raw"].as<String>();
                    }
                    if (amsList[0].containsKey("dry_time")) {
                        config.liveData.amsDryTime = amsList[0]["dry_time"].as<String>();
                    }
                    if (amsList[0].containsKey("temp")) {
                        config.liveData.amsDryTemp = amsList[0]["temp"].as<String>();
                    }
                    JsonArray trays = amsList[0]["tray"];
                    for (JsonObject tray : trays) {
                        if (tray.containsKey("id")) {
                            int id = tray["id"].as<String>().toInt();
                            if (id >= 0 && id < 4) {
                                if (tray.containsKey("tray_color")) config.liveData.ams[id].color = tray["tray_color"].as<String>();
                                if (tray.containsKey("tray_type")) config.liveData.ams[id].type = tray["tray_type"].as<String>();
                                if (tray.containsKey("remain")) amsRemain[id] = tray["remain"].as<int>();
                                
                                // Prioritize tray_info_idx for 100% accurate brand mapping
                                if (tray.containsKey("tray_info_idx") && tray["tray_info_idx"].as<String>().length() > 0) {
                                    String idx = tray["tray_info_idx"].as<String>();
                                    String subBrand = tray.containsKey("tray_sub_brands") ? tray["tray_sub_brands"].as<String>() : "";
                                    amsBrand[id] = getFilamentBrand(idx, subBrand);
                                } else if (tray.containsKey("tray_sub_brands")) {
                                    String brand = tray["tray_sub_brands"].as<String>();
                                    String brandLower = brand;
                                    brandLower.toLowerCase();
                                    amsBrand[id] = (brandLower.indexOf("bambu") >= 0) ? "Bambu" : "Generic";
                                }
                            }
                        }
                    }
                }
                }
            }
            
            if (print.containsKey("hms")) {
                JsonArray hmsList = print["hms"].as<JsonArray>();
                for (JsonVariant v : hmsList) {
                    JsonObject hmsNode = v.as<JsonObject>();
                    if (hmsNode.containsKey("attr")) {
                        uint32_t attr = hmsNode["attr"].as<uint32_t>();
                        char hexStr[20];
                        sprintf(hexStr, "%04X-%04X", (attr >> 16), (attr & 0xFFFF));
                        String codeStr = String(hexStr);
                        
                        bool found = false;
                        for (auto& err : hmsHistory) {
                            if (err.code == codeStr) { found = true; break; }
                        }
                        if (!found) {
                            Serial.printf("[HMS] New error detected: %s\n", hexStr);
                            hmsHistory.push_back({codeStr, millis(), true});
                            hasNewHmsError = true;
                        }
                    }
                }
            }
                
                if (print.containsKey("lights_report")) {
                    JsonArray lights = print["lights_report"].as<JsonArray>();
                    for (JsonVariant v : lights) {
                        JsonObject light = v.as<JsonObject>();
                        if (light["node"] == "chamber_light") {
                            chamberLightOn = (light["mode"] == "on");
                        }
                    }
                }
        }
    } else {
        Serial.print("[MQTT] JSON Parse Error: ");
        Serial.println(error.c_str());
    }
}