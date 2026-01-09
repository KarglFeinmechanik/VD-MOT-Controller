#include <Arduino.h>

#include "valve.h"
#include "mqtt.h"
#include "config.h"
#include "pins.h"

// ----------------- EXTERNS aus main.cpp -----------------
extern Config config;

extern bool valveMoving;
extern bool calibrating;
extern bool mqttReady;

extern float valveStartPos;
extern float valveEndPos;
extern float rehomeRecoveredPos;
extern float rehomeCorrection;

extern unsigned long valveMoveStart;
extern unsigned long valveMoveDuration;
extern unsigned long rehomeStart;
extern unsigned long rehomeOpenTime;
extern unsigned long rehomeCloseDuration;

extern int maxTravelTime;

extern RehomeState rehomeState;
extern MoveSource lastMoveSource;

// Helper aus main.cpp
extern void stopValvePins();
extern void updateStallLed(bool stall);
extern bool debouncePin(uint8_t pin, bool expectedLevel, unsigned long ms);
extern void ensureSaneTravelTime();
extern void saveConfig();
extern void setValveStatus(const char* s);

// -----------------------------------------------------------------------------
//                           VALVE CONTROL
// -----------------------------------------------------------------------------
void moveValve(float newTarget) {
    newTarget = constrain(newTarget, 0.0f, 1.0f);

    float delta = fabsf(newTarget - config.currentValvePos);
    if (delta < 0.01f) {
        Serial.println("Ignoring tiny valve move (<1%).");
        return;
    }

    config.targetValvePos = newTarget;

    valveStartPos     = config.currentValvePos;
    valveEndPos       = newTarget;
    valveMoveDuration = (unsigned long)(delta * (float)config.valveCloseTime);
    valveMoveStart    = millis();
    valveMoving       = true;

    if (valveEndPos > valveStartPos) {
        digitalWrite(PIN_CLOSE_VALVE, LOW);
        digitalWrite(PIN_OPEN_VALVE,  HIGH);
    } else {
        digitalWrite(PIN_OPEN_VALVE,  LOW);
        digitalWrite(PIN_CLOSE_VALVE, HIGH);
    }

    char buf[64];
    if (lastMoveSource == 2)      sprintf(buf, "MQTT: Moving to %.1f%%", newTarget * 100.0f);
    else if (lastMoveSource == 1) sprintf(buf, "UI: Moving to %.1f%%", newTarget * 100.0f);
    else                          sprintf(buf, "Moving to %.1f%%", newTarget * 100.0f);

    setValveStatus(buf);
}

// -----------------------------------------------------------------------------
//                           VALVE CALIBRATION
// -----------------------------------------------------------------------------
void calibrateValve() {

    mqttReady = false;

    setValveStatus("Calibration Started");
    delay(500);

    // -------- OPEN --------
    setValveStatus("Moving OPEN until Stall");
    digitalWrite(PIN_OPEN_VALVE, HIGH);
    digitalWrite(PIN_CLOSE_VALVE, LOW);

    unsigned long tOpen = millis();
    while (!debouncePin(PIN_STALL_DETECT, LOW, 80) &&
           millis() - tOpen < maxTravelTime) {
        delay(5);
        yield();
    }

    updateStallLed(true);
    stopValvePins();
    delay(300);

    // -------- CLOSE --------
    setValveStatus("Moving CLOSE until Stall");
    digitalWrite(PIN_CLOSE_VALVE, HIGH);
    digitalWrite(PIN_OPEN_VALVE, LOW);

    unsigned long tCloseStart = millis();
    while (!debouncePin(PIN_STALL_DETECT, LOW, 80) &&
           millis() - tCloseStart < maxTravelTime) {
        delay(5);
        yield();
    }

    config.valveCloseTime = millis() - tCloseStart;
    stopValvePins();

    // -------- FINALIZE --------
    config.currentValvePos = 0.0f;
    config.targetValvePos  = 0.0f;

    ensureSaneTravelTime();
    saveConfig();

    mqttPublishTeleFloat("position", 0.0f, 1, true);
    mqttPublishTeleFloat("targetpos", 0.0f, 1, true);


    setValveStatus("Moving to 50%");
    moveValve(0.5f);
    
    config.calibrated = true;
    saveConfig();

    mqttReady = true;
    updateStallLed(false);
}

// -----------------------------------------------------------------------------
//                           REHOME
// -----------------------------------------------------------------------------
void startRehome() {
    mqttReady = false;

    if (!config.calibrated) {
        Serial.println("Rehome skipped — not calibrated.");
        return;
    }

    if (digitalRead(PIN_STALL_DETECT) == LOW ||
        digitalRead(PIN_FAULT_DETECT) == LOW) {
        setValveStatus("Rehome ERROR (Driver Issue) ⚠️");
        rehomeState = REHOME_ERROR;
        return;
    }

    stopValvePins();
    valveMoving = false;

    setValveStatus("Rehome: Moving OPEN until Stall");
    digitalWrite(PIN_OPEN_VALVE, HIGH);
    digitalWrite(PIN_CLOSE_VALVE, LOW);

    rehomeStart = millis();
    rehomeState = REHOME_OPENING; // REHOME_OPENING
}

// -----------------------------------------------------------------------------
//                           REHOME HANDLER
// -----------------------------------------------------------------------------
void rehomeHandler() {
    switch (rehomeState) {

        case REHOME_OPENING: { // OPENING
            unsigned long elapsed = millis() - rehomeStart;
            if (elapsed > config.valveCloseTime + 3000) {
                stopValvePins();
                setValveStatus("Rehome ERROR ⚠️");
                rehomeState = REHOME_ERROR;
                return;
            }

            if (debouncePin(PIN_STALL_DETECT, LOW, 80)) {
                stopValvePins();
                rehomeOpenTime = elapsed;
                config.currentValvePos = 1.0f;
                mqttPublishTeleFloat("position", 100.0f, 1, true);
                rehomeState = REHOME_COMPUTE;
            }
        } break;

        case REHOME_COMPUTE: { // COMPUTE
            float percent = ((float)(config.valveCloseTime - rehomeOpenTime) / (float)config.valveCloseTime) * 100.0f;
            percent *= rehomeCorrection;
            percent = constrain(percent, 0.0f, 100.0f);
            rehomeRecoveredPos = percent / 100.0f;
            config.currentValvePos = rehomeRecoveredPos;

            if (percent >= 99.9f) {
                setValveStatus("Rehome Completed ✅");
                rehomeState = REHOME_DONE;
                return;
            }

            rehomeCloseDuration = rehomeOpenTime;
            mqttPublishTeleFloat("targetpos", rehomeRecoveredPos * 100.0f, 1, true);
            digitalWrite(PIN_OPEN_VALVE, LOW);
            digitalWrite(PIN_CLOSE_VALVE, HIGH);
            rehomeStart = millis();
            rehomeState = REHOME_CLOSING;
        } break;

        case REHOME_CLOSING: { // CLOSING
            if (debouncePin(PIN_STALL_DETECT, LOW, 80) ||
                millis() - rehomeStart >= rehomeCloseDuration) {
                stopValvePins();
                setValveStatus("Rehome Completed ✅");
                rehomeState = REHOME_DONE;
            }
        } break;

        case REHOME_DONE: { // DONE
            config.currentValvePos = rehomeRecoveredPos;
            config.targetValvePos  = rehomeRecoveredPos;
            mqttPublishTeleFloat("position",  rehomeRecoveredPos * 100.0f, 1, true);
            setValveStatus("Done Rehoming");
            delay(1000);
            mqttReady = true;
            setValveStatus("Idle");
            rehomeState = REHOME_IDLE;
        } break;

        case REHOME_ERROR: {
            stopValvePins();
            mqttReady = true;
            setValveStatus("Rehome Error ⚠️");
            rehomeState = REHOME_IDLE;
        } break;

        case REHOME_IDLE: { // IDLE
            default:
                break;
        } 

    }
}
