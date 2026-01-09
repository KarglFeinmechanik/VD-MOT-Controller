#pragma once

#include <Arduino.h>

// ----- API -----
void setValveStatus(const char* s);
void moveValve(float newTarget);
void calibrateValve();
void startRehome();
void rehomeHandler();

enum RehomeState : uint8_t {
    REHOME_IDLE = 0,
    REHOME_OPENING,
    REHOME_COMPUTE,
    REHOME_CLOSING,
    REHOME_ERROR,
    REHOME_DONE
};

enum MoveSource {
    MOVE_INTERNAL,
    MOVE_UI,
    MOVE_MQTT
};

