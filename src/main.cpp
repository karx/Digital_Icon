#include<Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Parola_Fonts_data.h"
#include<Font_Data.h>
#include <YoutubeApi.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // This Sketch doesn't technically need this, but the library does so it must be installed.
#include <Update.h>

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   14
#define DATA_PIN  23
#define CS_PIN    15

#define API_KEY "AIzaSyBQeMMEWAZNErbkgtcvF6iaJFW4237Vkfw"  // your google apps API Token
#define CHANNEL_ID "UC_vcKmg67vjMP7ciLnSxSHQ" //

const char* mqtt_server = "api.akriya.co.in";

// Variables to validate
// response from S3
int contentLength = 0;
bool isValidContentType = false;

void 	displayScroll (char *pText, textPosition_t align, textEffect_t effect, uint16_t speed);
void mqttCallback(char *topic, byte* payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient mqttClient(mqtt_server, 1883, mqttCallback, wifiClient);

const uint16_t WAIT_TIME = 1000;

// Hardware SPI connection
// MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary output pins
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
WiFiClientSecure client;
YoutubeApi api(API_KEY, client);

unsigned long api_mtbs = 10000; //mean time between api requests
unsigned long api_lasttime;   //last time api request has been done

long subs = 0;
 WiFiManager wifiManager;

char mo[75];
String msg = "";

 String host = "ytkarta.s3.ap-south-1.amazonaws.com"; // Host => bucket-name.s3.region.amazonaws.com
int port = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
String bin = "/kaaroMerch/SubsCount/firmware.bin"; // bin file name with a slash in front.
// https://ytkarta.s3.ap-south-1.amazonaws.com/kaaroMerch/SubsCount/firmware.bin
// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}



void execOTA() {
  Serial.println("Connecting to: " + String(host));
  // Connect to S3
  if (client.connect(host.c_str(), port)) {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching Bin: " + String(bin));

    // Get the contents of the bin file
    client.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
       Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Cache-Control: no-cache\r\n" +
                    "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        client.stop();
        return;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}
    */
    while (client.available()) {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);

    // If yes, begin
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    client.flush();
  }
}

// uint32_t bswqweqwe(String payload,int len)
// {
//     uint32_t i=0;
//     uint32_t result = 0;
//     for(i = 0;i<len;i++)
//     {
//         result *= 10;
//         result += (char)payload[i]-'0';
//     }
//     // Serial.print("Result Returning = ");
//     // Serial.println(result);
//     return result;
// }
// void getCount(String payld){

// if (flag == true ){
//    tcount = bswqweqwe(payld, payld.length());

// }
// }

void mqttCallback(char *topic, uint8_t* payload, unsigned int length) {
  char* cleanPayload = (char*)malloc(length+1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length+1);
   msg = String(cleanPayload);
  free(cleanPayload);

  String topics = String(topic);
  Serial.print("From MQTT = ");
  Serial.println(msg);


  

if(msg.equals("ota")){
Serial.println("Ota Initiating.........");
      execOTA();
}
if (topics == "digitalicon/"){
  P.setFont(ExtASCII);
  delay(100);
  P.displayText(msg.c_str(), PA_CENTER, 70, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

}
if (topics == "digitalicon/amit/count"){
      P.setFont(numeric7Seg);
      P.displayText(msg.c_str(), PA_CENTER, 70, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("subscounter", "Ready!");
      // ... and resubscribe
      mqttClient.subscribe("digitalicon/ota");
      mqttClient.subscribe("digitalicon/");
      mqttClient.subscribe("digitalicon/amit/count");

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {

    Serial.begin(115200);
    P.begin();
    P.setInvert(false);
    P.print("YoYo");            // Starting text on matrix
 
  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  wifiManager.setConnectTimeout(120);

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect("Digital Icon");    // SSID of config portal

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);


  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
  
}



void loop() {


  if (WiFi.status() == WL_CONNECTED) {
    
      if(!mqttClient.connected()){
    reconnect();
      }
  }

  else if (WiFi.status() != WL_CONNECTED){
      wifiManager.autoConnect("Digital Icon");
  }


  mqttClient.loop();
  wifiManager.process();

}







