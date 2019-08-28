
#ifndef ota.cpp
#define ota.cpp

class OTA_ESP32
{   public:
    static void execOTA(String host, int port, String bin, WiFiClient *wifiClient);
};

#endif