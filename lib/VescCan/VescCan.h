#pragma once
#include <Arduino.h>
#include <driver/twai.h>

class VescCan {
public:
    VescCan(gpio_num_t tx_pin, gpio_num_t rx_pin, int baud = 500000);
    ~VescCan();

    bool isOpen() const;

    bool setDuty(uint8_t controller_id, float duty);
    bool setCurrent(uint8_t controller_id, float current);
    bool setRpm(uint8_t controller_id, int32_t rpm);
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
    int rpm_= 0;
};
