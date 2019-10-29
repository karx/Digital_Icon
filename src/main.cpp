#include <Arduino.h>
#include <SPI.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <elapsedMillis.h>

#include <ota.h>
#include "display_kaaro.h"
#include "kaaro_utils.cpp"

/* 
    STATICS
*/
const char *mqtt_server = "api.akriya.co.in";
const uint16_t WAIT_TIME = 1000;
#define BUF_SIZE 75

/* 
    FUNCTION DEFINATIONS
*/

void displayScroll(char *pText, textPosition_t align, textEffect_t effect, uint16_t speed);
void mqttCallback(char *topic, byte *payload, unsigned int length);

/* 
 REALTIME VARIABLES
*/

int contentLength = 0;
bool isValidContentType = false;

String host = "ytkarta.s3.ap-south-1.amazonaws.com"; // Host => bucket-name.s3.region.amazonaws.com
int port = 80;                                       // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
String bin = "/kaaroMerch/SubsCount/firmware.bin";   // bin file name with a slash in front.

char mo[75];
String msg = "";
String DEVICE_MAC_ADDRESS;
String ssid = "";
String pass = "";
byte mac[6];

int cases = 1;

int fxMode;

uint32_t current_counter = 0;
uint32_t target_counter = 0;

unsigned long delayStart = 0; // the time the delay started
bool delayRunning = false;
unsigned int interval = 30000;

/*
  HY Variable/Instance creation
*/

WiFiClient wifiClient;
PubSubClient mqttClient(mqtt_server, 1883, mqttCallback, wifiClient);
WiFiManager wifiManager;
WiFiClientSecure client;

DigitalIconDisplay display;
elapsedMillis timeElapsed;

void mqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
  char *cleanPayload = (char *)malloc(length + 1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length + 1);
  msg = String(cleanPayload);
  free(cleanPayload);

  String topics = String(topic);
  Serial.print("From MQTT = ");
  Serial.println(msg);

  if (topics == "digitalicon/ota" && msg == "ota")
  {
    Serial.println("Ota Initiating.........");

    OTA_ESP32::execOTA(host, port, bin, &wifiClient);
  }

  else if (topics == "digitalicon/ota/version")
  {
  }

  if (topics == "digitalicon/")
  {
    display.showCustomMessage(msg);
  }
  if (topics == "digitalicon/91springboards1/count")
  {
    Serial.println("From count topic");
    display.updateCounterValue(msg, true);
  }

  if (topics == "digitalicon/91springboards1/message")
  {
    Serial.println("From message topic");
    display.showCustomMessage(msg);
  }
}

void reconnect()
{

  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("connected");

      String readyTopic = "digitalicon/" + DEVICE_MAC_ADDRESS;
      mqttClient.publish(readyTopic.c_str(), "Ready!");
      mqttClient.publish("digitalicon", "Ready!");

      mqttClient.subscribe("digitalicon/ota");
      mqttClient.subscribe("digitalicon/");
      mqttClient.subscribe("digitalicon/91springboards1/message");
      String otaTopic = "digitalicon/ota/" + DEVICE_MAC_ADDRESS;
      mqttClient.subscribe(otaTopic.c_str());

      String msgTopic = "digitalicon/" + DEVICE_MAC_ADDRESS;
      mqttClient.subscribe(msgTopic.c_str());

      mqttClient.subscribe("digitalicon/91springboards1/count");
      String countTopic = "digitalicon/count" + DEVICE_MAC_ADDRESS;
      mqttClient.subscribe(countTopic.c_str());
    }

    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}

void setup()
{

  Serial.begin(115200);
  DEVICE_MAC_ADDRESS = KaaroUtils::getMacAddress();
  Serial.println(DEVICE_MAC_ADDRESS);
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[0], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.println(mac[5], HEX);

  display.setupIcon();
  display.updateCounterValue("0", true);

  Serial.print("Connecting Wifi: ");
  wifiManager.setConnectTimeout(5);

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.autoConnect("Digital Icon");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    IPAddress ip = WiFi.localIP();
    Serial.println(ip);

    ssid = WiFi.SSID();
    pass = WiFi.psk();
  }

  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
}

void loop()
{

  wifiManager.process();

  if (timeElapsed > interval)
  {
    Serial.print("From here");
    // display.showCustomMessage(" Total ");

    switch (cases)
    {
    case 1:
      display.stripe();
      cases = 2;
      break;
    case 2:
      display.spiral();
      cases = 3;
      break;
    case 3:
      display.showCustomMessage(" Cowork.Network.Grow ");
      cases = 4;
      break;
    case 4:
      display.bounce();
      cases = 1;
      break;
    }
    timeElapsed = 0;
  }

  if (WiFi.status() == WL_CONNECTED)
  {

    if (!mqttClient.connected())
    {
      reconnect();
    }
  }
  mqttClient.loop();
  display.loop();
}
