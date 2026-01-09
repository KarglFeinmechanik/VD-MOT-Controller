#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <DHTesp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <Update.h>
#include <algorithm>

#include "pins.h"
#include "config.h"
#include "version.h"
#include "webpage.h"
#include "mqtt.h"
#include "valve.h"
#include "esp_system.h"

RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR esp_reset_reason_t lastResetReason;

// ---------------------- Globale Variablen ----------------------
String buildVersion = "";

Config config;
bool useDht = false;
float ds18Temps[MAX_DS18B20] = {0};
int ds18Count = 0;
int maxTravelTime = 35000; // maxTravelTime for Valve in ms

AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);
bool mqttEnabled = false;   // default disabled unless IoBroker enables it
String mqttStatus = "Disconnected";
unsigned long wifiConnectedSince = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastMqttCheck = 0;
unsigned long wifiLostSince = 0;

DHTesp dht;
OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds18(&oneWire);
Preferences pref;

//unsigned long calibStart = 0;
bool calibRequested = false;
unsigned long lastStatusTime = 0;
bool wifiConnected = false;
String valveStatus = "Idle";

bool valveMoving = false;
unsigned long valveMoveStart = 0;
unsigned long valveMoveDuration = 0;
float valveStartPos = 0.0f;  // 0..1
float rehomeRecoveredPos = 0.0f; // 0..1
float valveEndPos = 0.0f;  // 0..1
unsigned long lastPosSaveCheck = 0;
float lastSavedCurrentPos = -1;
float lastSavedTargetPos  = -1;
const unsigned long POS_SAVE_INTERVAL = 20UL * 60UL * 1000UL; // 20 min

bool calibrating = false;
bool mqttReady = false;
bool otaFailed = false;
String logBuffer;

MoveSource lastMoveSource = MOVE_INTERNAL;
RehomeState rehomeState = REHOME_IDLE;

unsigned long rehomeStart = 0;
unsigned long rehomeOpenTime = 0;
unsigned long rehomeCloseDuration = 0;
float rehomeCorrection = 1.03f;   // 1.02f = +2 %, 0.98f = −2 %


// -----------------------------------------------------------------------------
//                           HELPER FUNCTIONS
// -----------------------------------------------------------------------------
void logLine(const String& s) {
    logBuffer += s + "\n";
    if (logBuffer.length() > 4000)
        logBuffer.remove(0, 1000);
}

// -----------------------------------------------------------------------------
String formatDuration(unsigned long ms)
{
    unsigned long sec = ms / 1000;
    unsigned long h = sec / 3600;
    unsigned long m = (sec % 3600) / 60;
    unsigned long s = sec % 60;

    char buf[20];
    sprintf(buf, "%02lu:%02lu:%02lu", h, m, s);
    return String(buf);
}

// -----------------------------------------------------------------------------
bool debouncePin(uint8_t pin, bool expectedLevel, unsigned long ms)
{
    static uint32_t lastChange[40] = {0}; // ESP32 has <40 GPIOs
    static uint8_t  lastState[40] = {HIGH};

    uint8_t state = digitalRead(pin);

    if (state != lastState[pin]) {
        lastState[pin] = state;
        lastChange[pin] = millis();
    }

    return (millis() - lastChange[pin] >= ms) && (state == expectedLevel);
}

// -----------------------------------------------------------------------------
void updateStallLed(bool stallActive)
{
    //Implementation not done yet, LED PIN 7 not lighting up.

    //digitalWrite(PIN_STALL_LED, stallActive ? HIGH : LOW);
}

// -----------------------------------------------------------------------------
// Valve status setter (UI + MQTT)
// -----------------------------------------------------------------------------
void setValveStatus(const char* s) {
    valveStatus = s;
    Serial.println("STATUS → " + valveStatus);
    // MQTT (centralized)
    mqttPublishTele("status", valveStatus.c_str(), true);
}

// Ensure valveCloseTime is sane
void ensureSaneTravelTime() {
    if (config.valveCloseTime < 10000 || config.valveCloseTime > maxTravelTime) {
        Serial.printf("Invalid valveCloseTime in NVS (%lu ms) → using default %lu ms\n",
                      config.valveCloseTime,
                      config.DEFAULT_TRAVEL_TIME);
        config.valveCloseTime = config.DEFAULT_TRAVEL_TIME;
    }
    Serial.printf("Loaded valveCloseTime = %lu ms\n", config.valveCloseTime);
}

// Save / load config ----------------------------------------------------------
void saveConfig() {
    pref.begin("cfg", false);
    pref.putBytes("data", &config, sizeof(Config));
    pref.end();
}

// -----------------------------------------------------------------------------
void loadConfig() {
    pref.begin("cfg", true);
    pref.getBytes("data", &config, sizeof(Config));
    pref.end();
    ensureSaneTravelTime();
}

// Stop valve motor outputs ----------------------------------------------------
void stopValvePins() {
    digitalWrite(PIN_OPEN_VALVE, LOW);
    digitalWrite(PIN_CLOSE_VALVE, LOW);
}

// Check allowed filenames for FS operations -----------------------------------
bool isAllowedFsFile(const String& name) {
    return (name == "background.jpg" ||
            name == "favicon-32x32.png" ||
            name == "style.css");
}

// Replace placeholders in HTML ------------------------------------------------
void fillHtmlPlaceholders(String &html) {
    while (html.indexOf("__SSID__")      >= 0) html.replace("__SSID__",      config.wifi_ssid);
    while (html.indexOf("__WPASS__")     >= 0) html.replace("__WPASS__",     config.wifi_pass);
    while (html.indexOf("__MQTT__")      >= 0) html.replace("__MQTT__",      config.mqtt_server);
    while (html.indexOf("__MCLIENT__")   >= 0) html.replace("__MCLIENT__",   config.mqtt_client_id);
    while (html.indexOf("__MUSER__")     >= 0) html.replace("__MUSER__",     config.mqtt_user);
    while (html.indexOf("__MPASS__")     >= 0) html.replace("__MPASS__",     config.mqtt_pass);
    while (html.indexOf("__SINT__")      >= 0) html.replace("__SINT__",      String(config.mqttStatusInterval));
    while (html.indexOf("__SEL_DHT__")   >= 0) html.replace("__SEL_DHT__",   strcmp(config.sensor_type, "DHT22")   == 0 ? "selected" : "");
    while (html.indexOf("__SEL_DS18__")  >= 0) html.replace("__SEL_DS18__",  strcmp(config.sensor_type, "DS18B20") == 0 ? "selected" : "");
    html.replace("__FWVER__", buildVersion);
}

// Build status JSON for /status ----------------------------------------------
String buildStatusJson() {
    float hum = isnan(config.humidity) ? 0.0f : config.humidity;

    String json = "{";

    json += "\"calibrated\":" + String(config.calibrated ? "true" : "false") + ",";
    json += "\"calrunning\":" + String(calibrating ? "true" : "false") + ",";
    json += "\"currentPos\":" + String(config.currentValvePos * 100.0f, 1) + ",";
    json += "\"targetPos\":"  + String(config.targetValvePos  * 100.0f, 1) + ",";
    json += "\"stall\":"      + String(digitalRead(PIN_STALL_DETECT) == LOW ? "true" : "false") + ",";
    json += "\"fault\":"      + String(digitalRead(PIN_FAULT_DETECT) == LOW ? "true" : "false") + ",";
    json += "\"travelTime\":" + String(config.valveCloseTime) + ",";
    json += "\"movement\":\"" + valveStatus + "\",";
    json += "\"humidity\":"   + String(hum, 2) + ",";

    // ---- TEMPERATURE ARRAY ----
    json += "\"temps\":[";
    int sensorNum = useDht ? 1 : ds18Count;
    for (int i = 0; i < sensorNum; i++) {
        float v = isnan(ds18Temps[i]) ? 0.0f : ds18Temps[i];
        json += String(v, 2);
        if (i < sensorNum - 1) json += ",";
    }
    json += "],";

    json += "\"fwVersion\":\"" + buildVersion + "\",";
    if (wifiConnectedSince > 0)
        json += "\"wifiSince\":\"" + formatDuration(millis() - wifiConnectedSince) + "\",";
        else
        json += "\"wifiSince\":\"0\",";
    json += "\"mqttStatus\":\"" + mqttStatus + "\",";
    json += "\"mqttEnabled\":" + String(mqttEnabled ? "true" : "false") + ",";

    json += "\"canRehome\":" + String((rehomeState == REHOME_IDLE && !valveMoving &&
                digitalRead(PIN_STALL_DETECT) == HIGH && digitalRead(PIN_FAULT_DETECT) == HIGH) ? "true" : "false") + ",";

    // ---- VALVE OUTPUT STATES ----
    int openPinState  = digitalRead(PIN_OPEN_VALVE);
    int closePinState = digitalRead(PIN_CLOSE_VALVE);
    bool isMoving = (openPinState == HIGH || closePinState == HIGH);

    json += "\"openPin\":"  + String(openPinState) + ",";
    json += "\"closePin\":" + String(closePinState) + ",";
    json += "\"isMoving\":" + String(isMoving ? "true" : "false");

    json += "}";

    return json;
}

// -----------------------------------------------------------------------------
//                               SENSORS
// -----------------------------------------------------------------------------
void initSensors() {
    if (strcmp(config.sensor_type, "DHT22") == 0) {
        dht.setup(PIN_ONEWIRE, DHTesp::DHT22);
        useDht = true;
    } else if (strcmp(config.sensor_type, "DS18B20") == 0) {
        ds18.begin();
        ds18Count = ds18.getDeviceCount();
        ds18.setWaitForConversion(false);
        if (ds18Count > MAX_DS18B20) ds18Count = MAX_DS18B20;
        Serial.printf("Found %d DS18B20 sensors\n", ds18Count);
        useDht = false;
    }
}

// -----------------------------------------------------------------------------
void readSensors() {
    static unsigned long lastDhtRead = 0;
    static unsigned long lastDsReq   = 0;
    // ---------- DHT22 ----------
    if (useDht) {
        if (millis() - lastDhtRead < 2500) return;
        lastDhtRead = millis();
        TempAndHumidity th = dht.getTempAndHumidity();
        if (!isnan(th.temperature))
            ds18Temps[0] = th.temperature + config.temp_offset;
        if (!isnan(th.humidity))
            config.humidity = th.humidity + config.hum_offset;
        ds18Count = 1;
        return;
    }
    // ---------- DS18B20 ----------
    if (millis() - lastDsReq > 1000) {
        ds18.requestTemperatures();
        lastDsReq = millis();
    }
    for (int i = 0; i < ds18Count; i++) {
        float t = ds18.getTempCByIndex(i);
        ds18Temps[i] = isnan(t) ? 0.0f : (t + config.temp_offset);
    }
    config.humidity = NAN;
}

// -----------------------------------------------------------------------------
//                               WIFI / AP
// -----------------------------------------------------------------------------
void reconnectWiFi() {
    unsigned long now = millis();
    // ---------- MQTT LOOP FIRST ----------
    if (wifiConnected && mqtt.connected()) {
        mqtt.loop();
    }
    yield();
    // ---------- WIFI CHECK ----------
    if (now - lastWifiCheck >= 10000) {
        lastWifiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            if (wifiConnected) {
                Serial.println("⚠️ WiFi lost");
                wifiConnected = false;
                mqttStatus = "WiFi Lost ⚠️";
                wifiLostSince = now;
            }
            WiFi.reconnect();
            if (now - wifiLostSince > 30000UL) {
                Serial.println("🔄 Restarting WiFi");
                WiFi.disconnect(true, true);
                delay(50);
                yield();
                WiFi.begin(config.wifi_ssid, config.wifi_pass);
                wifiLostSince = now;
            }
        } else {
            if (!wifiConnected) {
                Serial.println("✅ WiFi reconnected");
                wifiConnectedSince = now;
            }
            wifiConnected = true;
            wifiLostSince = 0;
        }
    }
    // ---------- MQTT RECONNECT ----------
    if (wifiConnected && !mqtt.connected()) {
        if (now - lastMqttCheck >= 10000) {
            lastMqttCheck = now;
            mqttStatus = "Reconnecting";
            mqttReconnect();   // OK: nur EIN Versuch
        }
    }
}

// -----------------------------------------------------------------------------
void startAP() {
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true, true);
    delay(200);
    String apName = "FBH-Control-C3";
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(apName.c_str(), "fbhcontrolc3");
    Serial.println("Starting Access Point mode...");
    Serial.print("SSID: ");
    Serial.println(apName);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    wifiConnected = false;
}

// -----------------------------------------------------------------------------
void setupWiFi() {
    if (strlen(config.wifi_ssid) == 0) {
        Serial.println("No WiFi credentials set!");
        startAP();
        return;
    }
    Serial.printf("Connecting to WiFi SSID: %s\n", config.wifi_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_ssid, config.wifi_pass);
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttempt < 15000) 
    {
        delay(250);
        yield();
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection failed!");
        startAP();
        return;
    }
    wifiConnected = true;
    wifiConnectedSince = millis();
    Serial.print("✅ WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
}

// -----------------------------------------------------------------------------
//                               WEBSERVER
// -----------------------------------------------------------------------------
void setupWeb() {
    DefaultHeaders::Instance().addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
    DefaultHeaders::Instance().addHeader("Expires", "0");
    // CSS from PROGMEM
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send_P(200, "text/css", STYLE_css);
    });
    // Static files from LittleFS
    server.serveStatic("/background.jpg",   LittleFS, "/background.jpg");
    server.serveStatic("/favicon-32x32.png", LittleFS, "/favicon-32x32.png");
    // Main page
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *req) {
        String html = MAIN_page;
        fillHtmlPlaceholders(html);
        req->send(200, "text/html", html);
    });
    // Save config
    server.on("/save", HTTP_POST, [&](AsyncWebServerRequest *req) {
        if (req->hasParam("ssid", true))     strcpy(config.wifi_ssid,   req->getParam("ssid",   true)->value().c_str());
        if (req->hasParam("pass", true))     strcpy(config.wifi_pass,   req->getParam("pass",   true)->value().c_str());
        if (req->hasParam("mqtt", true)) {
            String ipPort = req->getParam("mqtt", true)->value();
            ipPort.trim();
            strcpy(config.mqtt_server, ipPort.c_str());
        }
        if (req->hasParam("mclient", true))  strcpy(config.mqtt_client_id, req->getParam("mclient", true)->value().c_str());
        if (req->hasParam("muser", true))    strcpy(config.mqtt_user,      req->getParam("muser",   true)->value().c_str());
        if (req->hasParam("mpass", true))    strcpy(config.mqtt_pass,      req->getParam("mpass",   true)->value().c_str());
        if (req->hasParam("statusInterval", true)) {
            config.mqttStatusInterval = constrain(req->getParam("statusInterval", true)->value().toInt(), 100, 60000);
        }
        if (req->hasParam("sensor_type", true)) {
            strcpy(config.sensor_type, req->getParam("sensor_type", true)->value().c_str());
        }
        saveConfig();
        req->send(200, "text/html", "<h1>Saved. Rebooting...</h1>");
        delay(2000);
        ESP.restart();
    });
    // Valve endpoint
    server.on("/valve", HTTP_GET, [&](AsyncWebServerRequest *req) {
        if (rehomeState != REHOME_IDLE || calibrating) {
            Serial.println("WEB Target ignored: Busy (Rehome or Calibration)");
            req->send(409, "text/plain", "Busy");
            return;
        }
        if (req->hasParam("pos")) {
            String p       = req->getParam("pos")->value();
            float percent  = p.toFloat();     // 0–100
            percent        = constrain(percent, 0.0f, 100.0f);
            float pos      = percent / 100.0f;
            config.targetValvePos = pos;
            lastMoveSource = MOVE_UI;
            //Serial.printf("New Valve Target Position %.1f%% (from UI)\n", percent);
            moveValve(pos);
        }
        req->send(200, "text/plain", "OK");
    });
    server.on("/rehome", HTTP_GET, [](AsyncWebServerRequest *req) {
        if (valveMoving || calibrating || rehomeState != REHOME_IDLE) {
            req->send(409, "text/plain", "BUSY");
            return;
        }
        // Sicherheitscheck: kein Stall, kein Fault
        if (digitalRead(PIN_STALL_DETECT) == LOW ||
            digitalRead(PIN_FAULT_DETECT) == LOW) {

            setValveStatus("Rehome blocked (Fault/Stall)");
            req->send(409, "text/plain", "FAULT");
            return;
        }
        setValveStatus("Rehome requested");
        startRehome();
        req->send(200, "text/plain", "OK");
    });
    server.on("/calibrate", HTTP_GET, [&](AsyncWebServerRequest *req) {
        calibRequested = true;
        //calibStart     = millis();
        req->send(200, "text/plain", "Calibration started");
    });
    server.on("/reset_calibration", HTTP_GET, [](AsyncWebServerRequest *request){
        config.calibrated = false;
        config.valveCloseTime = config.DEFAULT_TRAVEL_TIME;
        saveConfig();
        setValveStatus("Calibration reset done");
        Serial.println("Calibration reset by user");
        request->send(200, "text/plain", "Calibration reset");
    });
    server.on("/set_calibration", HTTP_GET, [](AsyncWebServerRequest *request){
        config.calibrated = true;
        config.valveCloseTime = 25000;
        saveConfig();
        setValveStatus("Calibration set done");
        Serial.println("Calibration set by user");
        request->send(200, "text/plain", "Calibration set");
    });
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send(200, "text/plain", logBuffer);
    });
    server.on("/status", HTTP_GET, [&](AsyncWebServerRequest *req) {
        String json = buildStatusJson();
        req->send(200, "application/json", json);
    });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/plain", "Restarting...");
        delay(500);
        ESP.restart();
    });
    server.on("/factory_reset", HTTP_GET, [](AsyncWebServerRequest *req) {
        Preferences pref;
        pref.begin("cfg", false);
        pref.clear();
        pref.end();
        req->send(200, "text/plain", "Factory reset done. Restarting...");
        delay(1000);
        ESP.restart();
    });
    // OTA firmware update
    server.on("/update", HTTP_POST,

        // ===== POST finished =====
        [](AsyncWebServerRequest *req) {
            if (otaFailed || Update.hasError()) {
                req->send(500, "text/plain", "OTA FAILED");
            } else {
                req->send(200, "text/plain", "OTA OK");
                delay(300);
                ESP.restart();
            }
        },

        // ===== Upload handler =====
        [](AsyncWebServerRequest *req,
        const String& filename,
        size_t index, uint8_t *data,
        size_t len, bool final)
        {   
            if (index == 0) {
                otaFailed = false;
                Serial.printf("OTA Start: %s (%u bytes)\n",
                            filename.c_str(), req->contentLength());

                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                    otaFailed = true;
                    return;
                }
            }
            if (!filename.endsWith(".bin")) {
                otaFailed = true;
                return;
            }
            if (!otaFailed) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                    otaFailed = true;
                    return;
                }
            }
            if (final) {
                if (!otaFailed) {
                    if (!Update.end(true)) {
                        Update.printError(Serial);
                        otaFailed = true;
                    } else {
                        Serial.println("OTA Success!");
                    }
                }
            }
            yield();
        }
    );
    // Delete single file
    server.on("/delete_file", HTTP_GET, [](AsyncWebServerRequest *req) {
        if (!req->hasParam("name")) {
            req->send(400, "text/plain", "Missing filename");
            return;
        }
        String name = req->getParam("name")->value();
        name.trim();
        if (name.startsWith("/")) name.remove(0, 1);
        Serial.println("Delete request for: " + name);
        if (!isAllowedFsFile(name)) {
            req->send(403, "text/plain", "Forbidden file");
            return;
        }
        String path = "/" + name;
        if (LittleFS.exists(path)) {
            LittleFS.remove(path);
            req->send(200, "text/plain", "Deleted " + path);
        } else {
            req->send(404, "text/plain", "File not found");
        }
    });
    // Upload file
    server.on("/upload_file", HTTP_POST,
        [](AsyncWebServerRequest *req) {
            // simple redirect via JS so we send only once
            req->send(200, "text/html", "<script>location.href='/'</script>");
        },
        [](AsyncWebServerRequest *req, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!isAllowedFsFile(filename)) {
                Serial.println("Upload rejected: " + filename);
                return;
            }
            static File uploadFile;
            if (index == 0) {
                Serial.println("Upload start: " + filename);
                uploadFile = LittleFS.open("/" + filename, "w");
            }
            if (uploadFile) {
                uploadFile.write(data, len);
            }
            if (final) {
                if (uploadFile) uploadFile.close();
                Serial.println("Upload complete: " + filename);
            }
        }
    );
    // Delete entire FS
    server.on("/delete_fs", HTTP_GET, [](AsyncWebServerRequest *req) {
        LittleFS.format();
        req->send(200, "text/plain", "Filesystem deleted. Rebooting...");
        delay(500);
        ESP.restart();
    });
    server.begin();
}

String resetReasonToString(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:  return "Power-On";
        case ESP_RST_BROWNOUT: return "Brownout";
        case ESP_RST_SW:       return "Software";
        case ESP_RST_WDT:      return "Watchdog";
        case ESP_RST_PANIC:    return "Panic";
        default:               return "Other";
    }
}

// -----------------------------------------------------------------------------
//                               SETUP
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(300);

    if (config.mqtt_client_id[0] == '\0') {
        Serial.println("❌ ERROR: mqtt_client_id is empty!");
    }
    if (strlen(config.mqtt_topic) < 3) {
        Serial.println("⚠️ MQTT topic missing → skip publish");
        return;
    }

    Serial.printf("RESET REASON: %d\n", esp_reset_reason());
    bootCount++;
    lastResetReason = esp_reset_reason();

    buildVersion = generateBuildVersion();
    Serial.printf("Firmware Version: %s\n", buildVersion.c_str());

    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
    }

    loadConfig();
    initPins();
    delay(200); // allow signals to stabilize

    lastSavedCurrentPos = config.currentValvePos;
    lastSavedTargetPos  = config.targetValvePos;

    bool stallLow = (digitalRead(PIN_STALL_DETECT) == LOW);
    bool faultLow = (digitalRead(PIN_FAULT_DETECT) == LOW);

    if (stallLow && faultLow) {
        setValveStatus("Missing External Power ⚠️");
    }

    initSensors();

    setupWiFi();
    setupWeb();

    if (wifiConnected) {
        mqtt.setCallback(mqttCallback);
        mqttReady = true;
    }

    // Rehome ist jetzt MANUELL → bleibt aus
    //if (config.calibrated) {
    //    startRehome();
    //}
}

// -----------------------------------------------------------------------------
//                               LOOP
// -----------------------------------------------------------------------------
void loop() {
    delay(1); 
    // ---------------- WiFi / MQTT ----------------
    reconnectWiFi();

    // -----------------------------------------------------
    // REHOME HANDLER
    // -----------------------------------------------------
    if (rehomeState != REHOME_IDLE) {
        rehomeHandler();
        valveMoving = false;
        yield();
        return;
    }

    // -----------------------------------------------------
    // SMOOTH VALVE MOVEMENT HANDLER
    // -----------------------------------------------------
    unsigned long now = millis();
    if (valveMoving && rehomeState == REHOME_IDLE && !calibrating) {
        unsigned long elapsed = now - valveMoveStart;
        bool stall = debouncePin(PIN_STALL_DETECT, LOW, 80);

        updateStallLed(stall);

        if (digitalRead(PIN_FAULT_DETECT) == LOW) {
            setValveStatus("Driver Fault ⚠️");
        }

        if (elapsed >= valveMoveDuration || stall) {
            stopValvePins();
            valveMoving = false;

            if (stall) {
                config.currentValvePos = valveStartPos;
                setValveStatus("Stall Detected ⚠️");
            } else {
                config.currentValvePos = valveEndPos;
                setValveStatus("Valve Idle");
            }

            mqttPublishTele("stall", stall ? "true" : "false", true);
            mqttPublishTeleFloat("position", config.currentValvePos * 100.0f, 1, true);
            mqttPublishTele("calrunning", "false", true);
        }
        else {
            float t = (float)elapsed / (float)valveMoveDuration;
            config.currentValvePos =
                valveStartPos + (valveEndPos - valveStartPos) * t;
        }
    }

    // -----------------------------------------------------
    // CALIBRATION HANDLER
    // -----------------------------------------------------
    if (calibRequested) {
        calibRequested = false;
        calibrating    = true;
        rehomeState    = REHOME_IDLE;
        mqttPublishTele("calrunning", "true", true);
        calibrateValve();
        mqttPublishTeleUL("movingtime", config.valveCloseTime, true);
        mqttPublishTele("calrunning", "false", true);
        calibrating = false;
    }

    // -----------------------------------------------------
    // PERIODIC VALVE POSITION SAVING
    // -----------------------------------------------------
    if (now - lastPosSaveCheck >= POS_SAVE_INTERVAL) {
        lastPosSaveCheck = now;

        if (!valveMoving && rehomeState == REHOME_IDLE && !calibrating) {
            bool changed =
                fabs(config.currentValvePos - lastSavedCurrentPos) > 0.001f ||
                fabs(config.targetValvePos  - lastSavedTargetPos)  > 0.001f;

            if (changed) {
                saveConfig();
                lastSavedCurrentPos = config.currentValvePos;
                lastSavedTargetPos  = config.targetValvePos;

                Serial.println("✔️ Positions saved (20min check)");
                mqttPublishTele("posSaved", "true", true);
            }
        }
    }

    // -----------------------------------------------------
    // PERIODIC MQTT STATUS
    // -----------------------------------------------------
    if (now - lastStatusTime >= config.mqttStatusInterval) {
        publishStatus();
        lastStatusTime = now;
    }

    // -----------------------------------------------------
    // SENSOR READING
    // -----------------------------------------------------
    static unsigned long lastSensor = 0;
    if (now - lastSensor >= 5000) {
        readSensors();
        lastSensor = now;
    }
}

