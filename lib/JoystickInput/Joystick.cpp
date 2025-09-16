#include <LittleFS.h>
#include <Preferences.h>

#include "Joystick.h"


Joystick::Joystick(int pin, float vRef, float deadzone)
    : pin(pin), vRef(vRef), deadzone(deadzone) {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = 0;
    }
}

float Joystick::readVoltage(int pin) {
    int raw = analogRead(pin);
    return (raw / 4095.0f) * vRef;  
}

float Joystick::mapToRange(float voltage) {
    float value;
    if (voltage >= center) {
        value = (voltage - center) / (maxVal - center);
    } else {
        value = (voltage - center) / (center - minVal);
    }

    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;

    if (fabs(value) < deadzone) value = 0.0f;

    return value;
}

void Joystick::readerTask() {
    while (true) {
        float v = readVoltage(pin);

        buffer[index] = v;
        index = (index + 1) % BUFFER_SIZE;

        float sum = 0;
        for (int i = 0; i < BUFFER_SIZE; i++) sum += buffer[i];
        float avgVoltage = sum / BUFFER_SIZE;

        avgValue = mapToRange(avgVoltage);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void Joystick::taskWrapper(void* param) {
    Joystick* self = static_cast<Joystick*>(param);
    self->readerTask();
}

void Joystick::begin() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    // LittleFS mit Formatierung, falls fehlerhaft
    if(!LittleFS.begin(true)){
        Serial.println("LittleFS Fehler beim Mounten!");
    }

    // Preferences testen
    Preferences prefs;
    if(prefs.begin("joystick", false)){
        Serial.println("Preferences geöffnet!");
        prefs.end();
    } else {
        Serial.println("Preferences öffnen fehlgeschlagen!");
    }
    
    loadCalibration();
    
    xTaskCreatePinnedToCore(
        taskWrapper,
        "JoystickReader",
        4096,
        this,
        1,
        &taskHandle,
        1
    );
}

float Joystick::getValue() {
    if(!isnan(simulatedValue)) return simulatedValue;
    if (!isCalibrated()) return  NAN;  // ungültiger Wert, solange nicht kalibriert
    return avgValue;
}

float Joystick::getVoltage() {
    if(!isnan(simulatedValue)) return 1.65 + simulatedValue*1.65;

    if (!isCalibrated()) return  NAN;  // ungültiger Wert, solange nicht kalibriert

    float sum = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) sum += buffer[i];
    return sum / BUFFER_SIZE;
}

// --- Kalibrierung ---
void Joystick::calibrateCenter() {
    center = getVoltage();
    if (isCalibrated()) { saveCalibration(); }
}

bool Joystick::calibrateMin() {
    minVal = getVoltage();
    if (minVal >= center - 0.5 || center == -1) {
        minVal = -1;
        return false;
    }
    if (isCalibrated()) { saveCalibration(); }
        return true;
}

bool Joystick::calibrateMax() {
    maxVal = getVoltage();
    if (maxVal <= center + 0.5 || center == -1) {
        maxVal = -1;
        return false;
    }
    if (isCalibrated()) { saveCalibration(); }
    return true;
}

bool Joystick::resetCalibration() {
    minVal = -1;
    maxVal = -1;
    center = -1;
    
    Preferences prefs;
    prefs.begin("joystick", false);
    prefs.remove("min");
    prefs.remove("max");
    prefs.remove("center");
    prefs.end();

    return true;
}

bool Joystick::isCalibrated() {
    return minVal!=-1 && maxVal!=-1 && center!=-1;
}

void Joystick::saveCalibration() {
    Preferences prefs;
    prefs.begin("joystick", false);
    prefs.putInt("min", minVal);
    prefs.putInt("max", maxVal);
    prefs.putInt("center", center);
    prefs.end();
}

void Joystick::loadCalibration() {
    Preferences prefs;
    prefs.begin("joystick", true);
    minVal = prefs.getInt("min",-1);
    maxVal = prefs.getInt("max",-1);
    center = prefs.getInt("center",-1);
    prefs.end();
}

