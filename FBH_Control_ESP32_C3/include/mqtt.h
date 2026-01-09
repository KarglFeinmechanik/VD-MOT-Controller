#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

// -------------------------------------------------
// Global MQTT client (defined in main.cpp)
// -------------------------------------------------
extern PubSubClient mqtt;

// -------------------------------------------------
// MQTT core
// -------------------------------------------------
void mqttReconnect();
void mqttCallback(char* topic, byte* payload, unsigned int len);
void publishStatus();

// -------------------------------------------------
// Publish helpers (tele/<deviceId>/...)
// Defaults ONLY here
// -------------------------------------------------
void mqttPublishTele(const char* sub, const char* payload, bool retained = false);
void mqttPublishTeleFloat(const char* sub, float value, int decimals = 1, bool retained = false);
void mqttPublishTeleUL(const char* sub, unsigned long value, bool retained = false);
