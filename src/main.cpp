

#include <M5Atom.h>

#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

#include <RemoteXY.h>
// RemoteXY configurate
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =
    {255, 0, 0, 32, 0, 92, 0, 15, 26, 0,
     71, 56, 25, 0, 53, 53, 0, 28, 24, 135,
     0, 0, 0, 0, 0, 0, 32, 67, 0, 0,
     160, 65, 0, 0, 32, 65, 0, 0, 0, 64,
     64, 0, 67, 1, 29, 42, 45, 23, 2, 26,
     6, 67, 4, 1, 1, 32, 7, 2, 26, 11,
     67, 6, 72, 1, 27, 7, 2, 26, 11, 129,
     0, 35, 38, 34, 3, 28, 65, 114, 109, 105,
     110, 115, 32, 83, 112, 101, 101, 100, 111, 109,
     101, 116, 101, 114, 32, 58, 45, 41, 0};
// this structure defines all the variables and events of your control interface
struct
{

  // output variables
  float G_kmh;
  char T_kmh[6];   // string UTF8 end zero
  char T_Date[11]; // string UTF8 end zero
  char T_Time[11]; // string UTF8 end zero

  // other variable
  uint8_t connect_flag; // =1 if wire connected, else =0

} RemoteXY;
#pragma pack(pop)
CRemoteXY *remotexy;

// Modbus server include
#include "ModbusServerRTU.h"
#define SERVER_ID 25
uint16_t data = 0;
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);

#include "FreqCountESP.h"
uint8_t inputPin = 33;
uint16_t timerMs = 1000;

#include <TinyGPS++.h>
static const uint32_t GPSBaud = 9600;
// Creat The TinyGPS++ object.  创建GPS实例
TinyGPSPlus gps;

HardwareSerial ss(1); // GPS listening on Serial1

ModbusMessage FC03(ModbusMessage request)
{
  uint16_t address;       // requested register address
  uint16_t words;         // requested number of registers
  ModbusMessage response; // response message to be sent back

  // get request values
  request.get(2, address); // 2
  request.get(4, words);   // 4

  Serial.printf("------------------------ adress: %i  words:%i\n", address, words);
  // LOG_I("------------------------ adress: %i  words:%i\n", address, words);

  // Address and words valid? We assume 8 registers here for demo
  if (address && words && (address + words)) // <= SERVER_NUM_VALUES
  {
    M5.dis.drawpix(0, 0, 0xff3300); // RED
    // Looks okay. Set up message with serverID, FC and length of data
    Serial.printf("------------------------ serverID: %i  FC:%i length:%i \n", request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    // Fill response with requested data
    for (uint16_t i = address; i < address + words; ++i)
    {
      response.add(data++);
      Serial.printf("------------------------  response.add %i\n", data);
    }
  }
  else
  {
    // No, either address or words are outside the limits. Set up error response.
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  return response;
}

void setup()
{

  M5.begin(true, false, true);
  M5.dis.fillpix(0x00004f);

  remotexy = new CRemoteXY(
      RemoteXY_CONF_PROGMEM,
      &RemoteXY,
      "",
      new CRemoteXYStream_BluetoothSerial(
          "myRemoteXY" // REMOTEXY_BLUETOOTH_NAME
          ));

  // uint64_t chipid;
  // char chipname[256];
  // chipid = ESP.getEfuseMac();
  // sprintf(chipname, "Sensor1%04X", (uint16_t)(chipid >> 32));
  // Serial.printf("Bluetooth: %s\n", chipname);
  // SerialBT.begin(chipname);

  Serial.printf("\nModBus serverID: %3i\n\n", SERVER_ID);
  Serial2.begin(9600, SERIAL_8N1, GPIO_NUM_22, GPIO_NUM_19); // Modbus connection
  // Register served function code worker for server 1, FC 0x03
  MBserver.registerWorker(SERVER_ID, READ_HOLD_REGISTER, &FC03); // ID 0x1B??
  // Start ModbusRTU background task
  MBserver.start();

  Serial.println(" FreqCounterESP lib"); // Console print
  FreqCountESP.begin(inputPin, timerMs);

  ss.begin(9600, SERIAL_8N1, 32, 26);
  Serial.println(" TinYGPS++ lib"); // Console print
}

void loop()
{
  while (ss.available()) // feed serial GPS as often as possible!
    gps.encode(ss.read());

  remotexy->handler();
  RemoteXY.G_kmh = gps.speed.kmph();
  snprintf(RemoteXY.T_kmh, 6, "%3.1f", gps.speed.kmph());

  snprintf(RemoteXY.T_Date, 11, "%02i-%02i-%04i",
           gps.date.day(),
           gps.date.month(),
           gps.date.year());

  snprintf(RemoteXY.T_Time, 11, "%02i:%02i:%02i",
           gps.time.hour(),
           gps.time.minute(),
           gps.time.second());

  // SerialBT.printf("Km/h= %.1f \r\n", gps.speed.kmph());

  // Frequency counter with GPIO33
  static unsigned long Ftimer = 0;
  if (FreqCountESP.available())
  {
    unsigned long t = millis();
    uint32_t frequency = FreqCountESP.read();
    Serial.printf("\nTime %lu ms  -  Frequency %i Hz\n", t - Ftimer, frequency);
    Ftimer = t;
  }

  // transfer GPS data via BlueTooth
  static bool blink = false;
  static unsigned long GpsLoopPM = 0;
  unsigned long GpsLoopCM = millis();
  if (GpsLoopCM - GpsLoopPM >= (1000 * 1))
  {
    if ((blink = !blink))
    {
      M5.dis.drawpix(0, 0, 0x0000cc); // blue
    }
    else
    {
      M5.dis.drawpix(0, 0, 0x00ff00); // green
    }

    if (gps.location.isUpdated())
    {
      Serial.print(F("LOCATION   Fix Age="));
      Serial.print(gps.location.age());
      Serial.print(F("ms Raw Lat="));
      Serial.print(gps.location.rawLat().negative ? "-" : "+");
      Serial.print(gps.location.rawLat().deg);
      Serial.print("[+");
      Serial.print(gps.location.rawLat().billionths);
      Serial.print(F(" billionths],  Raw Long="));
      Serial.print(gps.location.rawLng().negative ? "-" : "+");
      Serial.print(gps.location.rawLng().deg);
      Serial.print("[+");
      Serial.print(gps.location.rawLng().billionths);
      Serial.print(F(" billionths],  Lat="));
      Serial.print(gps.location.lat(), 6);
      Serial.print(F(" Long="));
      Serial.println(gps.location.lng(), 6);
    }
    if (gps.date.isUpdated())
    {
      Serial.print(F("DATE       Fix Age="));
      Serial.print(gps.date.age());
      Serial.print(F("ms Raw="));
      Serial.print(gps.date.value());
      Serial.print(F(" Year="));
      Serial.print(gps.date.year());
      Serial.print(F(" Month="));
      Serial.print(gps.date.month());
      Serial.print(F(" Day="));
      Serial.println(gps.date.day());
    }
    if (gps.time.isUpdated())
    {
      Serial.print(F("TIME       Fix Age="));
      Serial.print(gps.time.age());
      Serial.print(F("ms Raw="));
      Serial.print(gps.time.value());
      Serial.print(F(" Hour="));
      Serial.print(gps.time.hour());
      Serial.print(F(" Minute="));
      Serial.print(gps.time.minute());
      Serial.print(F(" Second="));
      Serial.print(gps.time.second());
      Serial.print(F(" Hundredths="));
      Serial.println(gps.time.centisecond());
    }
    if (gps.speed.isUpdated())
    {
      Serial.print(F("SPEED      Fix Age="));
      Serial.print(gps.speed.age());
      Serial.print(F("ms Raw="));
      Serial.print(gps.speed.value());
      Serial.print(F(" Knots="));
      Serial.print(gps.speed.knots());
      Serial.print(F(" MPH="));
      Serial.print(gps.speed.mph());
      Serial.print(F(" m/s="));
      Serial.print(gps.speed.mps());
      Serial.print(F(" km/h="));
      Serial.println(gps.speed.kmph());
    }

    if (gps.course.isUpdated())
    {
      Serial.print(F("COURSE     Fix Age="));
      Serial.print(gps.course.age());
      Serial.print(F("ms Raw="));
      Serial.print(gps.course.value());
      Serial.print(F(" Deg="));
      Serial.println(gps.course.deg());
    }

    if (gps.altitude.isUpdated())
    {
      Serial.print(F("ALTITUDE   Fix Age="));
      Serial.print(gps.altitude.age());
      Serial.print(F("ms Raw="));
      Serial.print(gps.altitude.value());
      Serial.print(F(" Meters="));
      Serial.print(gps.altitude.meters());
      Serial.print(F(" Miles="));
      Serial.print(gps.altitude.miles());
      Serial.print(F(" KM="));
      Serial.print(gps.altitude.kilometers());
      Serial.print(F(" Feet="));
      Serial.println(gps.altitude.feet());
    }

    if (gps.satellites.isUpdated())
    {
      Serial.print(F("SATELLITES Fix Age="));
      Serial.print(gps.satellites.age());
      Serial.print(F("ms Value="));
      Serial.println(gps.satellites.value());
    }

    if (gps.hdop.isUpdated())
    {
      Serial.print(F("HDOP       Fix Age="));
      Serial.print(gps.hdop.age());
      Serial.print(F("ms raw="));
      Serial.print(gps.hdop.value());
      Serial.print(F(" hdop="));
      Serial.println(gps.hdop.hdop());
    }

    if (gps.charsProcessed() < 10)
      Serial.println(F("WARNING: No GPS data.  Check wiring."));

    // -------- GpsLoop end ----------------------------------------------------------------
    GpsLoopPM = GpsLoopCM;
  }
}
