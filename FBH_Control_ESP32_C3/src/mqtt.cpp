#include <Arduino.h>
#include <WiFi.h>
#include <algorithm>

#include "mqtt.h"
#include "config.h"
#include "pins.h"
#include "valve.h"

// ---- externe Globals ----
extern bool wifiConnected;
extern bool mqttEnabled;
extern bool mqttReady;
extern String mqttStatus;

extern Config config;
extern bool calibrating;
extern float ds18Temps[];
extern int ds18Count;

extern bool valveMoving;
extern unsigned long lastStatusTime;

extern int rehomeState;
extern int lastMoveSource;

extern void moveValve(float pos);
extern bool debouncePin(uint8_t pin, bool expectedLevel, unsigned long ms);

// -----------------------------------------------------------------------------
// Topic base
// -----------------------------------------------------------------------------
static const char* deviceId() {
    return config.mqtt_client_id;
}

static const char* topicId() {
    return (config.mqtt_topic[0] != '\0') ? config.mqtt_topic : DEFAULT_TOPIC;
}


// -----------------------------------------------------------------------------
static bool isCmd(const char* topic, const char* sub) {
    char buf[96];
    snprintf(buf, sizeof(buf), "cmnd/%s/%s", topicId(), sub);
    return strcmp(topic, buf) == 0;
}

// -----------------------------------------------------------------------------
// Publish helpers (NO String!)
// -----------------------------------------------------------------------------
void mqttPublishTele(const char* sub, const char* payload, bool retained) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (!wifiConnected || !mqtt.connected()) return;
    char topic[128];
    snprintf(topic, sizeof(topic), "tele/%s/%s", topicId(), sub);
    mqtt.publish(topic, payload, retained);
}

// -----------------------------------------------------------------------------
void mqttPublishTeleFloat(const char* sub, float value, int decimals, bool retained) {
    char buf[32];
    dtostrf(value, 0, decimals, buf);
    mqttPublishTele(sub, buf, retained);
}

// -----------------------------------------------------------------------------
void mqttPublishTeleUL(const char* sub, unsigned long value, bool retained) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", value);
    mqttPublishTele(sub, buf, retained);
}

// -----------------------------------------------------------------------------
// MQTT server parsing
// -----------------------------------------------------------------------------
static void parseMqttServer(const char* input, char* hostOut, uint16_t &portOut) {
    String s(input);
    s.trim();

    int idx = s.indexOf(':');
    if (idx < 0) {
        strcpy(hostOut, s.c_str());
        portOut = 1883;
        return;
    }

    strcpy(hostOut, s.substring(0, idx).c_str());
    portOut = s.substring(idx + 1).toInt();
    if (portOut == 0) portOut = 1883;
}

// -----------------------------------------------------------------------------
// MQTT reconnect (NON-BLOCKING)
// -----------------------------------------------------------------------------
void mqttReconnect() {
    if (mqtt.connected()) return;

    char host[64];
    uint16_t port;
    parseMqttServer(config.mqtt_server, host, port);
    mqtt.setServer(host, port);

    Serial.printf("🔄 MQTT connecting to %s:%u\n", host, port);

    if (!mqtt.connect(deviceId() , config.mqtt_user, config.mqtt_pass)) {
        mqttStatus = "Reconnecting";
        Serial.println("❌ MQTT failed");
        return;
    }

    // ---------- CONNECTED ----------
    mqttStatus = "Connected";
    Serial.println("✅ MQTT connected!");

    // ---------- SUBSCRIBE ----------
    char cmndTopic[96];
    snprintf(cmndTopic, sizeof(cmndTopic), "cmnd/%s/#", topicId());
    mqtt.subscribe(cmndTopic);

    // ---------- CREATE COMMAND TOPICS (RETAINED) ----------
    static bool firstConnect = true;
    if (firstConnect) {
        firstConnect = false;
        char topic[96];

        snprintf(topic, sizeof(topic), "cmnd/%s/enable", topicId());
        mqtt.publish(topic, "false", true);

        snprintf(topic, sizeof(topic), "cmnd/%s/targetpos", topicId());
        mqtt.publish(topic, "-1", true);

        snprintf(topic, sizeof(topic), "cmnd/%s/calibrate", topicId());
        mqtt.publish(topic, "false", true);

        snprintf(topic, sizeof(topic), "cmnd/%s/rehome", topicId());
        mqtt.publish(topic, "false", true);

        // ---------- TELE INIT ----------
        snprintf(topic, sizeof(topic), "tele/%s/enabled", topicId());
        mqtt.publish(topic, mqttEnabled ? "true" : "false", true);

        snprintf(topic, sizeof(topic), "tele/%s/movingtime", topicId());
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", config.valveCloseTime);
        mqtt.publish(topic, buf, true);
    }
}

// -----------------------------------------------------------------------------
// MQTT callback
// -----------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int len) {

    if (!mqttReady || calibrating) return;
    char msgBuf[128];
    unsigned int copyLen = (len < sizeof(msgBuf) - 1) ? len : (sizeof(msgBuf) - 1);
    memcpy(msgBuf, payload, copyLen);
    msgBuf[copyLen] = '\0';
    String msg(msgBuf);
    msg.trim();

    // ENABLE / DISABLE
    if (isCmd(topic, "enable")) {
        bool newState = (msg == "1" || msg.equalsIgnoreCase("true"));
        if (mqttEnabled != newState) {
            mqttEnabled = newState;
            mqttPublishTele("enabled", mqttEnabled ? "true" : "false", true);
            setValveStatus(mqttEnabled ? "MQTT Enabled ✅" : "MQTT Disabled ☑️");
        }   lastStatusTime = 0;
        return;
    }

    // TARGET POSITION
    if (isCmd(topic, "targetpos")) {
        if (!mqttEnabled) return;
        if (msg == "-1") return;

        float percent = msg.toFloat();
        if (isnan(percent)) return;

        percent = constrain(percent, 0.0f, 100.0f);
        lastMoveSource = 2; // MOVE_MQTT
        moveValve(percent / 100.0f);

        mqttPublishTeleFloat("targetpos", percent, 1, true);
        return;
    }

    // CALIBRATION
    if (isCmd(topic, "calibrate")) {
        if (!mqttEnabled) return;
        if (msg == "1" || msg.equalsIgnoreCase("true")) {
            extern bool calibRequested;
            calibRequested = true;
            setValveStatus("Calibration requested (MQTT)");
        }
    }

    // REHOME
    if (isCmd(topic, "rehome")) {
        if (!mqttEnabled) return;
        if (calibrating) return;
        if (msg == "1" || msg.equalsIgnoreCase("true")) {
            setValveStatus("Rehome requested (MQTT)");
            startRehome();
        }
    }
}

// -----------------------------------------------------------------------------
// Periodic status publish
// -----------------------------------------------------------------------------
void publishStatus() {
    if (!wifiConnected || !mqtt.connected()) return;
    if (ds18Count > 0) {
        mqttPublishTeleFloat("temperature", ds18Temps[0], 2, true);
    }
    mqttPublishTeleFloat("humidity", config.humidity, 2, true);
    mqttPublishTeleUL("movingtime", config.valveCloseTime, true);
    mqttPublishTele("stall", debouncePin(PIN_STALL_DETECT, LOW, 80) ? "true" : "false", true);
    mqttPublishTele("fault", debouncePin(PIN_FAULT_DETECT, LOW, 80) ? "true" : "false", true);
}



