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
    if (voltage >= centerV) {
        value = (voltage - centerV) / (maxV - centerV);
    } else {
        value = (voltage - centerV) / (centerV - minV);
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

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void Joystick::taskWrapper(void* param) {
    Joystick* self = static_cast<Joystick*>(param);
    self->readerTask();
}

void Joystick::begin() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

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
    if (!isCalibrated()) return  NAN;  // ungültiger Wert, solange nicht kalibriert
    return avgValue;
}

float Joystick::getVoltage() {
    if (!isCalibrated()) return  NAN;  // ungültiger Wert, solange nicht kalibriert

    float sum = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) sum += buffer[i];
    return sum / BUFFER_SIZE;
}

// --- Kalibrierung ---
void Joystick::calibrateCenter() {
    centerV = getVoltage();
    calibratedCenter = true;
}

void Joystick::calibrateMin() {
    minV = getVoltage();
    calibratedMin = true;
}

void Joystick::calibrateMax() {
    maxV = getVoltage();
    calibratedMax = true;
}

bool Joystick::isCalibrated() {
    return calibratedCenter && calibratedMin && calibratedMax;
}
