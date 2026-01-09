#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define MAX_DS18B20 8 // maximale Anzahl DS18B20 Sensoren
#define DEFAULT_TOPIC "FBHControl"

// -------------------------------------------------
// Struktur für Konfiguration
// -------------------------------------------------
struct Config {
    // WLAN
    char wifi_ssid[32] = "";
    char wifi_pass[32] = "";

    // MQTT
    char mqtt_server[32] = "";
    char mqtt_client_id[64] = "FBH-Controller";
    char mqtt_topic[32] = "VD-MOT";
    char mqtt_user[32] = "";
    char mqtt_pass[32] = "";

    // Sensorwahl: "DHT22" oder "DS18B20"
    char sensor_type[16] = "DHT22";

    // Sensor Offsets
    float temp_offset = 0.0f;
    float hum_offset = 0.0f;
    float humidity;

    // Valve Kalibrierung
    const unsigned long DEFAULT_TRAVEL_TIME = 30000;  // 30 seconds
    unsigned long valveCloseTime = 30000; // default 30s
    bool calibrated = false;

    // Ventilposition
    float currentValvePos; // 0 = closed, 1 = open
    float targetValvePos;  // Soll-Position vom Host

    // MQTT Status Interval
    unsigned long mqttStatusInterval = 1000; // default 1s
};

// Globale Sensorwerte DS18B20
extern float ds18Temps[MAX_DS18B20];
extern int ds18Count;

extern Config config;

void saveConfig();
void loadConfig();

#endif // CONFIG_H
