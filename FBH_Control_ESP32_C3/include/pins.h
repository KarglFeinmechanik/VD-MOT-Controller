#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// -------------------------------------------------
// GPIO Definitionen für ESP32-C3 SuperMini
// -------------------------------------------------

// Aktoren / Outputs (Ventil gesteuert über moveValve)
#define PIN_OPEN_VALVE    4   // GPIO4
#define PIN_CLOSE_VALVE   5   // GPIO5
#define PIN_STALL_LED     7   // GPIO7

// Inputs
#define PIN_STALL_DETECT  3   // GPIO3 -> Stall Detection (LOW = aktiv)
#define PIN_FAULT_DETECT  6   // GPIO6 -> Fault Detection (LOW = aktiv)

// OneWire / DHT Pin
#define PIN_ONEWIRE       2   // GPIO2

// -------------------------------------------------
// Initialisierung der Pins
// -------------------------------------------------
inline void initPins() {
    pinMode(PIN_OPEN_VALVE, OUTPUT);
    pinMode(PIN_CLOSE_VALVE, OUTPUT);

    pinMode(PIN_STALL_LED, OUTPUT);

    pinMode(PIN_STALL_DETECT, INPUT_PULLUP);  
    pinMode(PIN_FAULT_DETECT, INPUT_PULLUP);  
}

#endif // PINS_H
