/*
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .

Bluetooth affects esp-now master !!!!!
-> packets are lost!

If BT is connected, then serial an BT loosing packets!
If BT is disconnected (by phone) serial does NOT loos any packets!

WiFi and ESP-NOW is working...

// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .
// INFO                                                           .


*/

#include <Arduino.h>
#include <M5Atom.h>

#include "esp_wifi.h"

#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

#include "espNowFloodingMeshLibrary2/EspNowFloodingMesh.h"

enum Sensor_ID
{
  SENSOR_FUEL = 12,
  SENSOR_TACHOMETER,
  SENSOR_GPS
};
struct Fuel_Sensor_t
{
  uint8_t SensorID;
  uint32_t TelegramCounter;  // internal counter - inc from start up
  uint16_t NoOfImpulses;     // sum of impulses are sent
  uint16_t SumOfImpulseTime; // sum of duration of sent impulses
  float MilliLitters;        // sum of collected ml for this message frame
};
Fuel_Sensor_t Fuel_Sensor;

unsigned char secredKey[] = {0xB8, 0xF0, 0xF4, 0xB7, 0x4B, 0x1E, 0xD7, 0x1E, 0x8E, 0x4B, 0x7C, 0x8A, 0x09, 0xE0, 0x5A, 0xF1}; // AES 128bit
unsigned char iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

int ESP_NOW_CHANNEL = 1; // INFO     must be the same channel as WIFI or AP !!!    (https://randomnerdtutorials.com/esp32-esp-now-wi-fi-web-server/)
int bsid = 0x010101;
const int ttl = 3;

bool blink = false;

void espNowFloodingMeshRecv(const uint8_t *data, int len, uint32_t replyPrt)
{
  if (len > 0)
  {
    Serial.print("<=== received Data from Sensor ID:");

    // first check incomming sensor ID
    uint8_t *SensorID = (uint8_t *)data;
    Serial.println(*SensorID);

    // after identifing ID get the real data!
    if (*SensorID == SENSOR_FUEL)
    {
      Fuel_Sensor_t *Fuel_Sensor = (Fuel_Sensor_t *)data;

      Serial.printf("#: %6i %13.8f [ml]\n", Fuel_Sensor->TelegramCounter, Fuel_Sensor->MilliLitters);
      SerialBT.printf("#: %6i %13.8f [ml]\n", Fuel_Sensor->TelegramCounter, Fuel_Sensor->MilliLitters);
    }
    else
    {
      Serial.printf("unknown Sensor ID %i!\n", *SensorID);
    }


  }
}
void setup()
{
  M5.begin(true, true, true);
  Serial.begin(115200);

  M5.dis.drawpix(0, 0, 0xff0000); //

  // Serial.println("\nPush button to start analysis!");
  // while (true)
  // {
  //   M5.update();
  //   if (M5.Btn.wasPressed())
  //   { // if M5 Button was pressed, then also start...
  //     break;
  //   }
  // }

  int8_t power;
  // esp_wifi_set_max_tx_power(20);
  esp_wifi_get_max_tx_power(&power);
  Serial.printf("wifi power: %d \n", power);

  uint64_t chipid;
  char chipname[256];
  chipid = ESP.getEfuseMac();
  sprintf(chipname, "Master%04X", (uint16_t)(chipid >> 32));
  Serial.printf("Bluetooth: %s\n", chipname);
  SerialBT.begin(chipname);

  // Set device in AP mode to begin with
  espNowFloodingMesh_RecvCB(espNowFloodingMeshRecv);
  espNowFloodingMesh_secredkey(secredKey);
  espNowFloodingMesh_disableTimeDifferenceCheck();
  espNowFloodingMesh_setAesInitializationVector(iv);

  espNowFloodingMesh_setToMasterRole(true); // Set ttl to 3.

  espNowFloodingMesh_ErrorDebugCB([](int level, const char *str)
                                  {
                            if (level == 0) {
                               Serial.printf("ERROR %s", str);
                               SerialBT.printf("ERROR %s", str);
                            }
                            if (level == 1) {
                               Serial.printf("WRN   %s", str);
                               SerialBT.printf("WRN   %s", str);
                            }
                            if (level == 2) {
                               Serial.printf("INFO  %s", str);
                               SerialBT.printf("INFO  %s", str);
                            } });

  espNowFloodingMesh_begin(ESP_NOW_CHANNEL, bsid, true);
}

void loop()
{
  static unsigned long counter = 0;
  static unsigned long m = millis();
  if (m + 10000 < millis())
  {

    char Message[35]{};
    snprintf(Message, sizeof(Message), "Master: %lu", counter);

    espNowFloodingMesh_sendAndHandleReply((uint8_t *)&Message, sizeof(Message), 3, [](const uint8_t *data, int len)
                                          {
        if(len>0) {
          Serial.print(">");
          Serial.println((const char*)data);

          SerialBT.print(">");
          SerialBT.println((const char*)data);

        } });
    m = millis();
    counter++;

        // ######################
    if (blink)
    {
      M5.dis.drawpix(0, 0, 0x0000cc); // blue
    }
    else
    {
      M5.dis.drawpix(0, 0, 0x00ff00); // green
    }
    blink = !blink;
  }

  M5.update();
  espNowFloodingMesh_loop();
  // delay(10);
}