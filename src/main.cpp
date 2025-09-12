#include <Arduino.h>
#include "VescCan.h"

#define CAN_TX GPIO_NUM_21
#define CAN_RX GPIO_NUM_20

VescCan vesc(CAN_TX, CAN_RX, 500000);

void setup() {
  Serial.begin(115200);
  if (!vesc.isOpen()) {
    Serial.println("❌ Fehler beim Starten von CAN");
    while (true);
  }
  Serial.println("✅ CAN bereit");

  // Heartbeat automatisch alle 100ms senden
  vesc.startHeartbeatTask(1, 100);

  delay(1000);
  Serial.println("➡️ Sende Duty 0.2");
  vesc.setDuty(1, 0.2f);
}

void loop() {
  // andere Steuerbefehle nach Bedarf
  delay(2000);
  Serial.println("➡️ Sende RPM 1500");
  vesc.setRpm(1, 1500);
}
