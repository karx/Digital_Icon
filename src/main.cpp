# define DI_Version "2.0.0-kaaroCount-Snap"
#include <Arduino.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <time.h>
#include <stdio.h>
#include <PubSubClient.h>

#define USEOTA
  #ifdef USEOTA
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
#endif

#include "display_kaaro.h"
#include "kaaro_utils.cpp"

#include <Preferences.h>
/*
   Things for Wifi & Network
*/
const char* modes[] = { "NULL", "STA", "AP", "STA+AP" };
unsigned long mtime = 0;

// TEST OPTION FLAGS
bool TEST_CP         = true; // always start the configportal, even if ap found
int  TESP_CP_TIMEOUT = 10; // test cp timeout

bool TEST_NET        = true; // do a network test after connect, (gets ntp time)
bool ALLOWONDEMAND   = true; // enable on demand
int  ONDDEMANDPIN    = 0; // gpio for button  

void handleRoute();
void print_oled(String str,uint8_t size);
void wifiInfo();
void getTime();

void saveWifiCallback();
void configModeCallback (WiFiManager *myWiFiManager);
void saveParamCallback();
void bindServerCallback();


/*
  Things for Product
*/
const char *mqtt_server = "api.akriya.co.in";
const uint16_t WAIT_TIME = 1000;
unsigned long long current_loop_counter = 0;
uint16_t current_brain_counter = 0;

const unsigned long long brain_beat = 1000000;
#define BUF_SIZE 75

String META_ROOT = "Kento/present/";
String ROOT_MQ_ROOT = "digitalicon/";
String PRODUCT_MQ_SUB = "discordakcount/";
String MESSAGE_MQ_STUB = "message";
String COUNT_MQ_STUB = "count";
String CUSTOM_MQ_STUB = "config";
String OTA_MQ_SUB = "ota/";

String presenceTopic;
String presenceDemandTopic;

String rootTopic;
String readyTopic;

String otaTopic;

String productMessageTopic;
String productCountTopic;
String productConfigTopic;

String messageTopic;
String countTopic;
  

String PRODUCT_UNIQUE = " Messages Exchanged ";
String DEVICE_MAC_ADDRESS;



/* 
    FUNCTION DEFINATIONS
*/

void mqttCallback(char *topic, byte *payload, unsigned int length);
void mqttSetTopicValues();
void pushEveryLoop();


/* 
    HY Variable/Instance creation
*/
WiFiClient wifiClient;
WiFiManager wm;
PubSubClient mqttClient(mqtt_server, 1883, mqttCallback, wifiClient);
DigitalIconDisplay display;
Preferences preferences;


void mqttSetTopicValues() {
  presenceTopic = META_ROOT + DEVICE_MAC_ADDRESS;
  presenceDemandTopic = META_ROOT + "REPORT";
  
  rootTopic = ROOT_MQ_ROOT;
  readyTopic = ROOT_MQ_ROOT + DEVICE_MAC_ADDRESS;

  otaTopic = ROOT_MQ_ROOT + OTA_MQ_SUB + DEVICE_MAC_ADDRESS;

  productMessageTopic = ROOT_MQ_ROOT + PRODUCT_MQ_SUB + MESSAGE_MQ_STUB;
  productCountTopic = ROOT_MQ_ROOT + PRODUCT_MQ_SUB + COUNT_MQ_STUB;
  productConfigTopic = ROOT_MQ_ROOT + PRODUCT_MQ_SUB + CUSTOM_MQ_STUB;

  messageTopic = ROOT_MQ_ROOT + MESSAGE_MQ_STUB + '/' + DEVICE_MAC_ADDRESS;
  countTopic = ROOT_MQ_ROOT + COUNT_MQ_STUB + '/' + DEVICE_MAC_ADDRESS;
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
  char *cleanPayload = (char *)malloc(length + 1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length + 1);
  String msg = String(cleanPayload);
  free(cleanPayload);

  String topics = String(topic);
  Serial.printf("From MQTT = ");
  Serial.println(msg);

  // String countTopic = ROOT_MQ_ROOT + COUNT_MQ_STUB + DEVICE_MAC_ADDRESS;

  if (topics == presenceDemandTopic)
  {
    Serial.println(DI_Version);
    mqttClient.publish(presenceTopic.c_str(), (DI_Version + String(current_brain_counter)).c_str());
  }
  if (topics == rootTopic)
  {
    display.showCustomMessage(msg);
  }
  if (topics == productCountTopic)
  {
    Serial.println(msg + " | From count topic");
    uint32_t counterVal = display.updateCounterValue(msg, true);
    preferences.putUInt("target_counter", counterVal);
  }

  if (topics == productMessageTopic || topics == messageTopic)
  {
    Serial.println(msg + " | From message topic");
    display.showCustomMessage(msg);
  }

  if(topics == productConfigTopic) {
    if(msg == F("inUpdate")) {
      int new_val = display.updateTextAnimationIn();
      preferences.putUInt("text_in_anm", new_val);

    } else if (msg == String("outUpdate")) {
      int new_val = display.updateTextAnimationOut();
      preferences.putUInt("text_out_anm", new_val);

    }
  }
  
}


void reconnect()
{

  if (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("connected");

      String readyMessage = DEVICE_MAC_ADDRESS + " is Ready.";
      mqttClient.publish(readyTopic.c_str(), "Ready!");
      mqttClient.publish(rootTopic.c_str(), readyMessage.c_str());
      mqttClient.publish(presenceTopic.c_str(), "0");

      mqttClient.subscribe(rootTopic.c_str());
      mqttClient.subscribe(otaTopic.c_str());
      mqttClient.subscribe(presenceDemandTopic.c_str());
      mqttClient.subscribe(productMessageTopic.c_str());
      mqttClient.subscribe(productCountTopic.c_str());
      mqttClient.subscribe(productConfigTopic.c_str());

      mqttClient.subscribe(messageTopic.c_str());
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
void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:
  Serial.begin(115200);

  delay(1000);
  Serial.println("\n Starting");
  
  
  #ifdef WM_OLED
    init_oled();
  #endif
  display.setupIcon();
  preferences.begin("digitalicon", false);
  uint32_t target_counter = preferences.getUInt("target_counter", 499);
  int textInAnimation = preferences.getUInt("text_in_anm", 1);
  int textOutAnimation = preferences.getUInt("text_out_anm", 1);
  Serial.println("Boot setup with ");
  Serial.println(target_counter);
  char str[100];
  sprintf(str, "%d", target_counter);
  String s = str;
  display.updateCounterValue(s, true);
  display.updateTextAnimationIn(textInAnimation);
  display.updateTextAnimationOut(textOutAnimation);

  print_oled(F("startn"),2);
  wm.debugPlatformInfo();
  
  // invert theme, dark
  wm.setClass("invert");

  // setup some parameters
  WiFiManagerParameter custom_html("<h3>Get your Snacks!</h3>"); // only custom html
  // 
  const char _customHtml_checkbox[] = "type=\"checkbox\""; 
  WiFiManagerParameter custom_checkbox("checkbox", "my checkbox", "T", 2, _customHtml_checkbox, WFM_LABEL_AFTER);

  // callbacks
  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setSaveParamsCallback(saveParamCallback);

  // add all your parameters here
  wm.addParameter(&custom_html);
  wm.addParameter(&custom_checkbox);

  // set values later if you want
  custom_html.setValue("test",4);

  std::vector<const char *> menu = {"wifi","wifinoscan","info","param","close","sep","erase","restart","exit"};
  // wm.setMenu(menu); // custom menu, pass vector
  
  // wm.setParamsPage(true); // move params to seperate page, not wifi, do not combine with setmenu!

  // set country
  // setting wifi country seems to improve OSX soft ap connectivity, 
  // may help others as well, default is CN which has different channels
  wm.setCountry("IN"); 

  // set Hostname
  wm.setHostname("WIFIMANAGER_HOSTNAME");

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  wm.setConfigPortalTimeout(120);
  
  // This is sometimes necessary, it is still unknown when and why this is needed but it may solve some race condition or bug in esp SDK/lib
  // wm.setCleanConnect(true); // disconnect before connect, clean connect
  
  wm.setBreakAfterConfig(true);

  wifiInfo();

  if(!wm.autoConnect("WM_AutoConnectAP","12345678")) {
    Serial.println("failed to connect and hit timeout");
    print_oled("Not Connected",2);
  }
  else if(TEST_CP) {
    // start configportal always
    delay(1000);
    Serial.println("TEST_CP ENABLED");
    wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
    wm.startConfigPortal("WM_ConnectAP");
  }
  else {
    //if you get here you have connected to the WiFi
     Serial.println("connected...yeey :)");
      print_oled("Connected\nIP: " + WiFi.localIP().toString() + "\nSSID: " + WiFi.SSID(),1);    
  }
  
  wifiInfo();

  pinMode(ONDDEMANDPIN, INPUT_PULLUP);

  #ifdef USEOTA
    ArduinoOTA.begin();
  #endif

  DEVICE_MAC_ADDRESS = KaaroUtils::getMacAddress();
  mqttSetTopicValues(); 

  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
}

void wifiInfo(){
  WiFi.printDiag(Serial);
  Serial.println("SAVED: " + (String)wm.getWiFiIsSaved() ? "YES" : "NO");
  Serial.println("SSID: " + (String)wm.getWiFiSSID());
  Serial.println("PASS: " + (String)wm.getWiFiPass());
}

void loop() {
  #ifdef USEOTA
    ArduinoOTA.handle();
  #endif
  
  // is configuration portal requested?
  if (ALLOWONDEMAND && digitalRead(ONDDEMANDPIN) == LOW ) {
    delay(100);
    if ( digitalRead(ONDDEMANDPIN) == LOW ){
      Serial.println("BUTTON PRESSED");
      wm.setConfigPortalTimeout(140);
      wm.setParamsPage(false); // move params to seperate page, not wifi, do not combine with setmenu!

      // disable captive portal redirection
      // wm.setCaptivePortalEnable(false);
      
      if (!wm.startConfigPortal("OnDemandAP","12345678")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
      }
    }
    else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
      print_oled("Connected\nIP: " + WiFi.localIP().toString() + "\nSSID: " + WiFi.SSID(),1);    
      getTime();
    }
  }

  if(WiFi.status() == WL_CONNECTED && millis()-mtime > 10000 ){
    getTime();
    mtime = millis();
  }

  // put your main code here, to run repeatedly:
  
  if (WiFi.status() == WL_CONNECTED)
  {

    if (!mqttClient.connected())
    {
      reconnect();
    }
    mqttClient.loop();
    pushEveryLoop();
  }
  display.loop();
  delay(100);
}

void getTime() {
  int tz           = 5.5;
  int dst          = 0;
  time_t now       = time(nullptr);
  unsigned timeout = 5000;
  unsigned start   = millis();  
  configTime(tz * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  while (now < 8 * 3600 * 2 ) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
    if((millis() - start) > timeout){
      Serial.println("\n[ERROR] Failed to get NTP time.");
      return;
    }
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void debugchipid(){
  // WiFi.mode(WIFI_STA);
  // WiFi.printDiag(Serial);
  // Serial.println(modes[WiFi.getMode()]);
  
  // ESP.eraseConfig();
  // wm.resetSettings();
  // wm.erase(true);
  WiFi.mode(WIFI_AP);
  // WiFi.softAP();
  WiFi.enableAP(true);
  delay(500);
  // esp_wifi_start();
  delay(1000);
  WiFi.printDiag(Serial);
  delay(60000);
  ESP.restart();

  // AP esp_267751
  // 507726A4AE30
  // ESP32 Chip ID = 507726A4AE30
}

#ifdef WM_OLED
void init_oled(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }

  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixepl scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.display();
}

void print_oled(String str,uint8_t size){
  display.clearDisplay();
  display.setTextSize(size);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(str);
  display.display();
}
#else
  void print_oled(String str,uint8_t size){
    display.showCustomMessage(str);

    
  }
#endif



void saveWifiCallback(){
  Serial.println("[CALLBACK] saveCallback fired");
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("[CALLBACK] configModeCallback fired");
  #ifdef ESP8266
    print_oled("WiFiManager Waiting\nIP: " + WiFi.softAPIP().toString() + "\nSSID: " + WiFi.softAPSSID(),1); 
  #endif  
  // myWiFiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); 
  // Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  // Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  // wm.stopConfigPortal();
}

void bindServerCallback(){
  wm.server->on("/custom",handleRoute);
  // wm.server->on("/info",handleRoute); // you can override wm!
}

void handleRoute(){
  Serial.println("[HTTP] handle route");
  wm.server->send(200, "text/plain", "hello from user code");
}


void pushEveryLoop() {
  
  
  if(current_loop_counter%brain_beat == 0) {
    current_brain_counter++;
    mqttClient.publish(presenceTopic.c_str(), String(current_brain_counter).c_str());
  }
  current_loop_counter++;
}