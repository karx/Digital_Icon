#include <Arduino.h>
#include <display.h>
// #include <MD_Parola.h>
// #include <MD_MAX72xx.h>
#include <SPI.h>
// #include "Parola_Fonts_data.h"
// #include <Font_Data.h>
#include <YoutubeApi.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // This Sketch doesn't technically need this, but the library does so it must be installed.
#include <Update.h>
#include <ota.h>

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted

// #define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// #define MAX_DEVICES 4
// #define CLK_PIN 14
// #define DATA_PIN 23
// #define CS_PIN 15

#define API_KEY "AIzaSyBQeMMEWAZNErbkgtcvF6iaJFW4237Vkfw" // your google apps API Token
#define CHANNEL_ID "UC_vcKmg67vjMP7ciLnSxSHQ"             //

const char *mqtt_server = "api.akriya.co.in";

// Variables to validate
// response from S3
int contentLength = 0;
bool isValidContentType = false;

void displayScroll(char *pText, textPosition_t align, textEffect_t effect, uint16_t speed);
void mqttCallback(char *topic, byte *payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient mqttClient(mqtt_server, 1883, mqttCallback, wifiClient);

const uint16_t WAIT_TIME = 1000;

// Hardware SPI connection
// MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
WiFiClientSecure client;
YoutubeApi api(API_KEY, client);

unsigned long api_mtbs = 10000; //mean time between api requests
unsigned long api_lasttime;     //last time api request has been done

long subs = 0;
WiFiManager wifiManager;

char mo[75];
String msg = "";
String mac ;

String host = "ytkarta.s3.ap-south-1.amazonaws.com"; // Host => bucket-name.s3.region.amazonaws.com
int port = 80;                                       // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
String bin = "/kaaroMerch/SubsCount/firmware.bin";   // bin file name with a slash in front.

DigitalIconDisplay display;

String ssid ="";
String pass = "";

int fxMode;

uint32_t current_counter = 0;
uint32_t target_counter = 0;

uint8_t scrollSpeed = 25;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000; // in milliseconds

// Global message buffers shared by Serial and Scrolling functions
#define	BUF_SIZE	75
char curMessage[BUF_SIZE] = { "Booting Up" };
// char newMessage[BUF_SIZE] = { "Hello! Enter new message?" };
bool newMessageAvailable = true;

// https://ytkarta.s3.ap-south-1.amazonaws.com/kaaroMerch/SubsCount/firmware.bin
// Utility to extract header value from headers
// String getHeaderValue(String header, String headerName)
// {
//   return header.substring(strlen(headerName.c_str()));
// }

uint32_t getMacAddress1(const uint8_t *hwaddr1) {
	// uint8_t baseMac[6];
	// // Get MAC address for WiFi station
	// esp_read_mac(*baseMac, ESP_MAC_WIFI_STA);
	// // char baseMacChr[18] = {0};
	// // sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
	// return uint8_t(*baseMac);

    uint32_t value = 0;

    value |= hwaddr1[2] << 24; //Big endian (aka "network order"):
    value |= hwaddr1[3] << 16;
    value |= hwaddr1[4] << 8;
    value |= hwaddr1[5];
    // Serial.println(hwaddr1[5]);
    return value + 1;
}

String getMacAddress() {
	uint8_t baseMac[6];
	// Get MAC address for WiFi station
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  // Serial.println(baseMac[6]);
  uint8_t* fsas = (uint8_t*)baseMac;
  uint32_t value1 = getMacAddress1(fsas);
  Serial.println(value1);

	// char baseMacChr[18] = {0};
	// sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
	return String(value1);
}

// void updateCounter(int mode,String msg) {
//   switch (mode)
//   {
//   case 0:
//     P.setFont(ExtASCII);
//     delay(100);
//     P.displayText(msg.c_str(), PA_CENTER, 70, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
//     P.displayAnimate();
//     delay(1000);
//     Serial.println("Yayy");
//     break;
//   case 1: 
//     P.setFont(numeric7Seg);
//     // P.displayText(msg.c_str(), PA_CENTER, 70, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
//     P.print(msg.c_str());
//     Serial.print("Actual = ");
//     Serial.println(msg.c_str());
//     delay(1000);
//     break;
//   default:
//     break;
//   }
//   P.displayAnimate();

// }

String longlongToString(uint32_t ll) {
  char string[20];
  string[0] = '\0';
  string[1] = '0';

  int i = 0;
  while(ll) {
    string[i++] = '0' + ll%10;
    ll /= 10;
  }
  return String(string);
}
// void fxUpdate() {
//   if (fxMode == 0) {
//     uint32_t toShow = current_counter;
//     // Serial.println("\n\n\n\n");
//     // Serial.print("Current = ");
//     // Serial.println((double)toShow);
//     // Serial.print("Target = ");
//     // Serial.println(target_counter);

//     if (target_counter > current_counter) {
      
//       toShow += (target_counter - current_counter)/2 + 1;
//       // Serial.print("Sending = ");
//       // Serial.println(toShow);
//       current_counter = toShow;
//       // updateCounter(1, String(toShow));

//     } else {
//       toShow = target_counter;
//       // we make sure target_counter is being displayed
//     }


//   } else if (fxMode == 1) {
//     P.displayAnimate();
//   }
  
// }
uint32_t stoi(String payload,int len)
{
    uint32_t i=0;
    uint32_t result = 0;
    for(i = 0;i<len;i++)
    {
        result *= 10;
        result += (char)payload[i]-'0';
    }
    // Serial.print("Result Returning = ");
    // Serial.println(result);
    return result;
}


void setTargetCounter(String msg) {
  
  int val = stoi(msg, msg.length());
  Serial.println("val");
  Serial.println(val);
  target_counter = val;
}
// void getCount(String payld){

// if (flag == true ){
//    tcount = bswqweqwe(payld, payld.length());

// }
// }

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
    // client.setTimeout(3000);

    OTA_ESP32::execOTA(host,port,bin,&wifiClient);
  } 
  
  else if (topics == "digitalicon/ota/version" ) {

  }

  if (topics == "digitalicon/")
  {
    fxMode = 1;
    // updateCounter(0, msg);
  }
  if (topics == "digitalicon/amit/count")
  {
    fxMode = 0;
    
    setTargetCounter(msg);
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while(!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String readyTopic = "digitalicon/" + mac;
      mqttClient.publish( readyTopic.c_str(), "Ready!");
      mqttClient.publish( "digitalicon" , "Ready!");
      // Serial.println(readyTopic);

      // ... and resubscribe
      mqttClient.subscribe("digitalicon/ota");
      mqttClient.subscribe("digitalicon/");
      String otaTopic = "digitalicon/ota/" + mac;
      mqttClient.subscribe(otaTopic.c_str());

      String msgTopic = "digitalicon/" + mac;
      mqttClient.subscribe(msgTopic.c_str());

      mqttClient.subscribe("digitalicon/amit/count");
      String countTopic = "digitalicon/count" + mac;
      mqttClient.subscribe(countTopic.c_str());
    }
    // else if (WiFi.status() != WL_CONNECTED){
    //   break;
    // }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup()
{


  Serial.begin(115200);
  mac = getMacAddress();
  Serial.println(mac);
  // P.begin();
  // P.setInvert(false);
  // wifiManager.resetSettings();
  
  // P.print("Booting"); // Starting text on matrix
  //  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
  // P.displayText("YAROO", PA_CENTER, 70, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  // P.displayAnimate();

  display.setupIcon();
  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  wifiManager.setConnectTimeout(15);

  wifiManager.setConfigPortalBlocking(false);
 wifiManager.setWiFiAutoReconnect(true);
  wifiManager.autoConnect("Digital Icon"); // SSID of config portal
    // wm.setAPCallback(configModeCallback);

if (WiFi.status() == WL_CONNECTED){
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  // Serial.println(WiFiManager._ssid());
   ssid = WiFi.SSID();
   pass = WiFi.psk();


}


  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
}

void loop()
{
  // if(P.displayAnimate()){
  //     P.displayReset();
  // }
  wifiManager.process();

  if (WiFi.status() != WL_CONNECTED){

    //     // String ssid = WiFi.SSID();
    //     Serial.println(ssid.c_str());

        
    //     Serial.println(WiFi.psk().c_str());

    //     while (WiFi.status() != WL_CONNECTED) {
    //     WiFi.begin(ssid.c_str(),WiFi.psk().c_str());
    //     delay(1000);
    //     Serial.print(".");
    // }
  }

  if (WiFi.status() == WL_CONNECTED)
  {

    if (!mqttClient.connected())
    {
      reconnect();
    }
  }
  else {
    //  WiFi.begin(_ssid,_pass);
    //  Serial.println(_ssid);
  }

  // else if (WiFi.status() != WL_CONNECTED)
  // {
  //   wifiManager.autoConnect("Digital Icon");
  // }

  mqttClient.loop();
  // fxUpdate();

}
