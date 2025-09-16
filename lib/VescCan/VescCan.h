#pragma once
#include <Arduino.h>
#include <driver/twai.h>

enum class VescControlType : uint8_t {
    NONE = 0,
    DUTY = 1,
    CURRENT = 2,
    RPM = 3
};

class VescCan {
public:
    VescCan(gpio_num_t tx_pin, gpio_num_t rx_pin, int baud = 500000);
    ~VescCan();

    bool isOpen() const;

    bool setControl(uint8_t controller_id, VescControlType type, float value);
    // value wird je nach Typ interpretiert:
    // DUTY: -1.0..1.0, CURRENT: Ampere, RPM: Drehzahl

    bool sendDuty(uint8_t controller_id, float duty);
    bool sendCurrent(uint8_t controller_id, float current);
    bool sendRpm(uint8_t controller_id, int32_t rpm);
    

    // Heartbeat-API
    bool sendHeartbeat(uint8_t controller_id, int32_t state = 1, int32_t fault = 0);
    void startHeartbeatTask(uint8_t controller_id, int interval_ms = 100);
    void stopHeartbeatTask();

private:
    bool open_ok;
    void appendInt32BE(uint8_t *buf, int32_t val);
    bool sendCanFrame(uint32_t extended_id, const uint8_t *data, uint8_t len);
    

    // Task-Handling
    static void heartbeatTask(void *param);
    TaskHandle_t hbTaskHandle = nullptr;
    uint8_t hbControllerId = 1;
    int hbInterval = 100;
    VescControlType type_ = VescControlType::NONE;
    int value_= 0;
};
