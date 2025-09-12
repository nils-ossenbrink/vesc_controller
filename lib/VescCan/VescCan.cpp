#include "VescCan.h"

static const uint32_t CAN_PACKET_SET_DUTY     = 0;
static const uint32_t CAN_PACKET_SET_CURRENT  = 1;
static const uint32_t CAN_PACKET_SET_RPM      = 3;
static const uint32_t CAN_PACKET_HEARTBEAT    = 9;

VescCan::VescCan(gpio_num_t tx_pin, gpio_num_t rx_pin, int baud) : open_ok(false) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_pin, rx_pin, TWAI_MODE_NORMAL);

    twai_timing_config_t t_config;
    if (baud == 250000) {
        t_config = TWAI_TIMING_CONFIG_250KBITS();
    } else if (baud == 1000000) {
        t_config = TWAI_TIMING_CONFIG_1MBITS();
    } else {
        t_config = TWAI_TIMING_CONFIG_500KBITS();
    }

    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK &&
        twai_start() == ESP_OK) {
        open_ok = true;
    }
}

VescCan::~VescCan() {
    stopHeartbeatTask();
    if (open_ok) {
        twai_stop();
        twai_driver_uninstall();
    }
}

bool VescCan::isOpen() const {
    return open_ok;
}

void VescCan::appendInt32BE(uint8_t *buf, int32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8)  & 0xFF;
    buf[3] = val & 0xFF;
}

bool VescCan::sendCanFrame(uint32_t extended_id, const uint8_t *data, uint8_t len) {
    if (!open_ok) return false;
    twai_message_t msg{};
    msg.identifier = extended_id;
    msg.extd = 1;
    msg.rtr = 0;
    msg.data_length_code = len;
    if (len > 0) memcpy(msg.data, data, len);
    return (twai_transmit(&msg, pdMS_TO_TICKS(50)) == ESP_OK);
}

bool VescCan::setDuty(uint8_t controller_id, float duty) {
    int32_t v = (int32_t)(duty * 100000.0f);
    uint8_t buf[4];
    appendInt32BE(buf, v);
    uint32_t eid = (CAN_PACKET_SET_DUTY << 8) | controller_id;
    return sendCanFrame(eid, buf, 4);
}

bool VescCan::setCurrent(uint8_t controller_id, float current) {
    int32_t v = (int32_t)(current * 1000.0f);
    uint8_t buf[4];
    appendInt32BE(buf, v);
    uint32_t eid = (CAN_PACKET_SET_CURRENT << 8) | controller_id;
    return sendCanFrame(eid, buf, 4);
}

bool VescCan::setRpm(uint8_t controller_id, int32_t rpm) {
    uint8_t buf[4];
    appendInt32BE(buf, rpm);
    uint32_t eid = (CAN_PACKET_SET_RPM << 8) | controller_id;
    return sendCanFrame(eid, buf, 4);
}

bool VescCan::sendHeartbeat(uint8_t controller_id, int32_t state, int32_t fault) {
    uint8_t buf[8];
    appendInt32BE(buf, state);
    appendInt32BE(buf+4, fault);
    uint32_t eid = (CAN_PACKET_HEARTBEAT << 8) | controller_id;
    return sendCanFrame(eid, buf, 8);
}

// -------- Hintergrund-Heartbeat --------
void VescCan::startHeartbeatTask(uint8_t controller_id, int interval_ms) {
    stopHeartbeatTask(); // evtl. altes stoppen
    hbControllerId = controller_id;
    hbInterval = interval_ms;

    xTaskCreatePinnedToCore(
        heartbeatTask,
        "vesc_heartbeat",
        2048,
        this,
        1,
        &hbTaskHandle,
        1
    );
}

void VescCan::stopHeartbeatTask() {
    if (hbTaskHandle) {
        vTaskDelete(hbTaskHandle);
        hbTaskHandle = nullptr;
    }
}

void VescCan::heartbeatTask(void *param) {
    auto *self = static_cast<VescCan*>(param);
    while (true) {
        self->sendHeartbeat(self->hbControllerId, 1, 0);
        vTaskDelay(pdMS_TO_TICKS(self->hbInterval));
    }
}
