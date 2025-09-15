#include <Arduino.h>
#include "VescCan.h"
#include "SerialCommands.h"
#include "Joystick.h"
#include <esp32_can.h>

#define CAN_TX GPIO_NUM_14
#define CAN_RX GPIO_NUM_13

VescCan vesc(CAN_TX, CAN_RX, 500000);

Joystick js(34);

char serial_command_buffer_[32];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

//This is the default handler, and gets called when no other command matches. 
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
	sender->GetSerial()->print("Unrecognized command [");
	sender->GetSerial()->print(cmd);
	sender->GetSerial()->println("]");
}

void printFrame(CAN_FRAME *message)
{
  Serial.print(message->id, HEX);
  if (message->extended) Serial.print(" X ");
  else Serial.print(" S ");   
  Serial.print(message->length, DEC);
  Serial.print(" ");
  for (int i = 0; i < message->length; i++) {
    Serial.print(message->data.byte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

//expects one single parameter
void cmd_set_rpm(SerialCommands* sender)
{
	//Note: Every call to Next moves the pointer to next parameter

	char* rpm_str = sender->Next();
	if (rpm_str == NULL)
	{
		sender->GetSerial()->println("ERROR WRONG PARAMETER");
		return;
	}

	int rpm = atoi(rpm_str);
	
  vesc.setRpm(1, rpm);

	sender->GetSerial()->print("RPM set to ");
	sender->GetSerial()->print(rpm);
}

SerialCommand cmd_set_rpm_("rpm", cmd_set_rpm);

void setup() {
  Serial.begin(115200);
  if (!vesc.isOpen()) {
    printf("❌ Fehler beim Starten von CAN");
    while (true);
  }
  Serial.println("✅ CAN bereit");

  js.begin();
  
  // Heartbeat automatisch alle 100ms senden
  vesc.startHeartbeatTask(1, 500);

  delay(1000);
  
  serial_commands_.SetDefaultHandler(cmd_unrecognized);
	serial_commands_.AddCommand(&cmd_set_rpm_);

  Serial.println("Ready ...!");
}

void loop() {
  static unsigned long lastTime = 0;
  unsigned long now = millis();

  serial_commands_.ReadSerial();

  
  if (now - lastTime >= 100) {  // alle 100ms
      lastTime = now;

      float val = js.getValue();
      float volt = js.getVoltage();

      if (isnan(val) || isnan(volt)) {
          Serial.println("Joystick noch nicht kalibriert!");
      } else {
          Serial.printf("Normiert: %.2f   Spannung: %.2f V\n", val, volt);
      }
  }
}
