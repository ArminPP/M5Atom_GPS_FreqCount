

#include <Arduino.h>
#include <M5Atom.h>

#include <esp_wifi.h>
#include <WiFi.h>
// #include <DNSServer.h>
#include <aWOT.h>

#include "espNowFloodingMeshLibrary2/EspNowFloodingMesh.h"

// INFO  must be in credentials or so ...                                                                                       .
// INFO  must be in credentials or so ...                                                                                       .

struct MeshProbe_struct
{
  char name[15]; // name of the mesh slave
  uint32_t TimeStamp;
  float MPU_Temperature;

  bool MPU_Engine_on; // Magnetometer

  float MPU_ax_rms; // Acceleration MPU
  float MPU_ax_min;
  float MPU_ax_max;
  float MPU_ay_rms;
  float MPU_ay_min;
  float MPU_ay_max;
  float MPU_az_rms;
  float MPU_az_min;
  float MPU_az_max;

  int16_t aX_FFT1; // Acceleration FFT
  int16_t aX_FFT2;
  int16_t aX_FFT3;
  int16_t aX_FFT4;
  int16_t aX_FFT5;
  int16_t aX_FFT6;
  int16_t aX_FFT7;
  int16_t aX_FFT8;
  int16_t aX_FFT9; // Acceleration FFT
  int16_t aX_FFT10;
  int16_t aX_FFT11;
  int16_t aX_FFT12;
  int16_t aX_FFT13;
  int16_t aX_FFT14;
  int16_t aX_FFT15;
  int16_t aX_FFT16;
  int16_t aX_FFT17; // Acceleration FFT
  int16_t aX_FFT18;
  int16_t aX_FFT19;
  int16_t aX_FFT20;
  int16_t aX_FFT21;
  int16_t aX_FFT22;
  int16_t aX_FFT23;
  int16_t aX_FFT24;
  int16_t aX_FFT25; // Acceleration FFT
  int16_t aX_FFT26;
  int16_t aX_FFT27;
  int16_t aX_FFT28;
  int16_t aX_FFT29;
  int16_t aX_FFT30;
  int16_t aX_FFT31;
  int16_t aX_FFT32;

  // 512 TFT bins ?? // TODO
  // INFO MESH payload is < 250 bytes !!!!!!!!!!!!!!!!
};
MeshProbe_struct MeshProbe;

unsigned char secredKey[] = {0xB8, 0xF0, 0xF4, 0xB7, 0x4B, 0x1E, 0xD7, 0x1E, 0x8E, 0x4B, 0x7C, 0x8A, 0x09, 0xE0, 0x5A, 0xF1}; // AES 128bit
unsigned char iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

int ESP_NOW_CHANNEL = 11; // INFO     must be the same channel as WIFI or AP !!!    (https://randomnerdtutorials.com/esp32-esp-now-wi-fi-web-server/)
int bsid = 0x010101;
const int ttl = 3;

// --------------------------------------------------------------
// Replace with your network credentials
const char *wifi_ssid = "ESP32-AP";
const char *wifi_pw = "123456789";

// const char *wifi_ssid = "_W_";
// const char *wifi_pw = "33V246o*";

// const char *w_ssid = "OnePlus 6";
// const char *w_password = "FlapperSp0t";

// DNSServer dnsServer;
// Set web server port number to 80
WiFiServer server(80);
Application app;
char redirectURL[30];
// --------------------------------------------------------------

// INFO  must be in credentials or so ...                                                                                       .
// INFO  must be in credentials or so ...                                                                                       .

int32_t getWiFiChannel(const char *ssid)
{

  if (int32_t n = WiFi.scanNetworks())
  {
    for (uint8_t i = 0; i < n; i++)
    {
      if (!strcmp(ssid, WiFi.SSID(i).c_str()))
      {
        return WiFi.channel(i);
      }
    }
  }

  return 0;
}

void espNowFloodingMeshRecv(const uint8_t *data, int len, uint32_t replyPrt)
{
  // if (len > 0)
  // {
  //   Serial.println((const char *)data);
  // }
  Serial.print("<=== received Data :");
  Serial.println((const char *)data);
  // Serial.println((int)data);
}

void initWIFIforMesh(int32_t channel)
// If using ESP-Now then the ESP-Now has
// to be the same as used for WiFi Channel.
//
// This can be a problem, if the WiFi AP is switching channels...
//
{
  // WiFi.mode(WIFI_MODE_STA);
  WiFi.mode(WIFI_MODE_APSTA);
  // int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial);
}

String Sruntime()
// FIX   this function (with millis()) rolls over after 47 days.!!!
// FIX   use ezTime instead? save startup time and subtract it from actual time !!!
// FIX   ##########################################################################
{
  // char *runtime()
  //  static char buf[40]{}; // return value must be static char[]

  //  snprintf(buf, sizeof(buf), "%.04d:%.02d:%.02d", hours(t), minutes(t), seconds(t));
  //  return buf;
  String Time = "";
  String ms = "";
  unsigned long ss, MS;
  byte mm, hh;

  MS = millis();
  ss = millis() / 1000;
  hh = ss / 3600;
  mm = (ss - hh * 3600) / 60;
  ss = (ss - hh * 3600) - mm * 60;
  if (hh < 10)
    Time += "0";
  Time += (String)hh + ":";
  if (mm < 10)
    Time += "0";
  Time += (String)mm + ":";
  if (ss < 10)
    Time += "0";
  Time += (String)ss + ":";

  ms = (String)MS;
  if (MS >= 1000)
  {
    ms = ms.substring(ms.length() - 3);
  };

  Time += ms;
  return Time;
}

// void redirect(Request &req, Response &res)
// {
//   if (!res.statusSent())
//   {
//     res.set("Location", redirectURL);
//     res.sendStatus(302);
//   }
// }

void index(Request &req, Response &res)
{
  res.printf("Captive portal index"); // @ %s", WiFi.localIP());
}

// void popup(Request &req, Response &res)
// {
//   res.print("Captive portal popup");
// }

void setup()
{
  M5.begin(true, true, true);
  Serial.begin(115200);

  Serial.println("\nPress some serial key or M5 Button to start program 1"); // DEBUG
  while (Serial.available() == 0)
  {
    M5.update();
    if (M5.Btn.wasPressed())
    { // if M5 Button was pressed, then also start...
      break;
    }
  }
  Serial.println("OK"); // DEBUG
                        /*
                          espNowFloodingMesh_secredkey(secredKey);
                          espNowFloodingMesh_setAesInitializationVector(iv);
                          espNowFloodingMesh_setToMasterRole(false, ttl);
                          espNowFloodingMesh_setToBatteryNode(false);
                      
                          WiFi.mode(WIFI_AP_STA);
                          WiFi.printDiag(Serial); // Uncomment to verify channel number before
                          esp_wifi_set_promiscuous(true);
                          esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
                          esp_wifi_set_promiscuous(false);
                          WiFi.printDiag(Serial); // Uncomment to verify channel change after
                      
                          WiFi.softAP(wifi_ssid, wifi_pw, ESP_NOW_CHANNEL); // NEW
                          IPAddress IP = WiFi.softAPIP();                   //
                          Serial.print("AP IP address: ");                  //
                          Serial.println(IP);                               //
                                                                            //
                      
                          // Serial.println("\nPress some serial key or M5 Button to start program 1a"); // DEBUG
                          // while (Serial.available() == 0)
                          // {
                          //   M5.update();
                          //   if (M5.Btn.wasPressed())
                          //   { // if M5 Button was pressed, then also start...
                          //     break;
                          //   }
                          // }
                          // Serial.println("OK"); // DEBUG
                      
                          // espNowFloodingMesh_begin(ESP_NOW_CHANNEL, bsid, false);
                      
                          espNowFloodingMesh_ErrorDebugCB([](int level, const char *str)
                                                          {
                            if (level == 0) {
                               Serial.printf("ERROR %s", str);
                            }
                            if (level == 1) {
                               Serial.printf("WRN   %s", str);
                            }
                            if (level == 2) {
                               Serial.printf("INFO  %s", str);
                            } });
                      
                          Serial.println("\nPress some serial key or M5 Button to start program 2"); // DEBUG
                          while (Serial.available() == 0)
                          {
                            M5.update();
                            if (M5.Btn.wasPressed())
                            { // if M5 Button was pressed, then also start...
                              break;
                            }
                          }
                        */
                        // WiFi.mode(WIFI_MODE_AP);
                        // WiFi.softAP(wifi_ssid, wifi_pw, ESP_NOW_CHANNEL); // NEW

  // INFO FUNKT!!                                                                                      .
  // WiFi.begin(w_ssid, w_password);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(1000);
  //   Serial.println("Setting as a Wi-Fi Station..");
  // }

  // INFO                                                                                              .
  /*
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(wifi_ssid, wifi_pw);//, ESP_NOW_CHANNEL); // NEW

    app.get("/", &index);   //
    server.begin();         // NEW
    WiFi.printDiag(Serial); // Uncomment to verify channel change after

    // IPAddress IP = WiFi.softAPIP();                   //
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.softAPIP());

    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel()); //

  //*/

  int32_t channel = getWiFiChannel(wifi_ssid);
  Serial.print("Wi-Fi Channel before Init: ");
  Serial.println(channel);

  initWIFIforMesh(11);

  // WiFi.softAP("TestNetwork", "123456789");
  espNowFloodingMesh_secredkey(secredKey);
  espNowFloodingMesh_setAesInitializationVector(iv);
  espNowFloodingMesh_disableTimeDifferenceCheck();
  espNowFloodingMesh_setToMasterRole(false, ttl);
  espNowFloodingMesh_setToBatteryNode(false);
  espNowFloodingMesh_RecvCB(espNowFloodingMeshRecv);

  // WiFi.begin(); // NEW from USS
  // WiFi.mode(WIFI_MODE_STA);
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.printDiag(Serial);

  espNowFloodingMesh_begin(ESP_NOW_CHANNEL, bsid, false);

  espNowFloodingMesh_ErrorDebugCB([](int level, const char *str)
                                  {
                            if (level == 0) {
                               Serial.printf("ERROR %s", str);
                            }
                            if (level == 1) {
                               Serial.printf("WRN   %s", str);
                            }
                            if (level == 2) {
                               Serial.printf("INFO  %s", str);
                            } });

  WiFi.softAP(wifi_ssid, wifi_pw, ESP_NOW_CHANNEL); // NEW
  // IPAddress ip = WiFi.softAPIP();
  // sprintf(redirectURL, "http://%d.%d.%d.%d/popup", ip[0], ip[1], ip[2], ip[3]);
  // Serial.println(ip);

  // WiFi.begin(wifi_ssid, wifi_pw, ESP_NOW_CHANNEL); // NEW
  // WiFi.printDiag(Serial);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());

  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  // WiFi.softAPConfig()

  app.get("/", &index);
  // app.get("/popup", &popup);
  // app.use(&redirect);

  server.begin();

  // dnsServer.start(53, "*", ip);

  Serial.println("OK"); // DEBUG
}

void loop()
{
  espNowFloodingMesh_loop();

  WiFiClient client = server.available();

  if (client.connected())
  {
    Serial.println("Client connected");
    app.process(&client);
    client.stop();
  }
  // dnsServer.processNextRequest();

  // ---------------------------------------------------------------------------------
  static bool blink = false;
  static int iCount = 0;
  static unsigned long MeshLoopPM = 0;
  unsigned long MeshLoopCM = millis();
  if (MeshLoopCM - MeshLoopPM >= 2000) // sending mesh values

  {
    if ((blink = !blink))
    {
      M5.dis.drawpix(0, 0, 0x0000cc); // blue
    }
    else
    {
      M5.dis.drawpix(0, 0, 0x00ff00); // green
    }

    strlcpy(MeshProbe.name, "SLAVE 1", sizeof(MeshProbe.name));
    MeshProbe.MPU_ax_rms = (millis() * 0.1);

    MeshProbe.TimeStamp = iCount++;

    espNowFloodingMesh_send((uint8_t *)&MeshProbe, sizeof(MeshProbe), ttl); // set ttl to 3

    Serial.print("---> packet sent: ");
    Serial.print(MeshProbe.name);
    Serial.print(" ,  ");
    Serial.print(MeshProbe.MPU_ax_rms);
    Serial.print(" ,  ");
    Serial.print(MeshProbe.TimeStamp);
    Serial.print(" ,  ");
    Serial.println(Sruntime());

    // -------- MeshLoop end --------
    MeshLoopPM = MeshLoopCM;
  }
  // */
}