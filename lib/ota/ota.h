
#ifndef ota_h
    #define ota_h
#include <Arduino.h>
#include <WiFi.h>

class OTA_ESP32
{   public:
    static void execOTA(String host, int port, String bin, WiFiClient *wifiClient);
};

#endif