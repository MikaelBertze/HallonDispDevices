#include "wifi-credentials.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>  
#include <ArduinoOTA.h>
#include <ledcontrol.h>
#include <tempreporter.h>
#include <doorreporter.h>
#include <powerreporter.h>


#define RED_LED D4
#define BLUE_LED D2
#define GREEN_LED D3

#ifndef IOT_ID
  #error “IOT_ID not specified. Define this in platform.ini for your target”
#endif 

#ifndef TOPIC_SPACE
  #error “TOPIC_SPACE not specified. Define this in platform.ini for your target”
#endif 



#define DEBOUNCE_MS 50
volatile long lastDebounceTime = 0;
volatile long lastTick = -1;

const char* mqtt_server = BROKER;
//const char* mqtt_garage_id = "garageThingy1";
const char* mqtt_temp_id = IOT_ID "_temp";
const char* mqtt_door_id = IOT_ID "_door";
const char* mqtt_power_id = IOT_ID "_power";


ESP8266WiFiMulti WiFiMulti;
LedControl ledControl(RED_LED, GREEN_LED, BLUE_LED, false);
TempReporter tempReporter(D1, IOT_ID);
DoorReporter doorReporter(D6, IOT_ID);
PowerReporter powerReporter(D5, IOT_ID);

long tempReportLastSend = 0;
long doorReportLastSend = 0;
long powerReportLastSend = 0;


void reboot() {
  ledControl.RebootSignal();
  ESP.restart();
}

void ICACHE_RAM_ATTR handleInterrupt ();

void handleInterrupt() {
  // Check to see if the change is within a debounce delay threshold.
  boolean debounce = (millis() - lastDebounceTime) <= DEBOUNCE_MS;

  long timeNow = millis();
  // This update to the last debounce check is necessary regardless of debounce state.
  lastDebounceTime = timeNow;

  // Ignore reads within a debounce delay threshold.
  if(debounce) return;

  powerReporter.SetTickPeriod(timeNow - lastTick);
  
  lastTick = timeNow;
}

void verifyWifi() {

  if (WiFi.status() == WL_CONNECTED)
    return;
  
  Serial.print("No connection...");
  ledControl.SetRed(true);
  byte num_retries = 60;
  byte retries = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    Serial.print("connecting...");
    delay(1000);
    ledControl.ToggleBlue();
    if(retries++ > num_retries){
      Serial.print("Restarting.");
      reboot();
    }
  }
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("ChipID: ");
  Serial.println(ESP.getChipId());
  ledControl.SetRed(false);
}

void setup() {
  ledControl.InitLeds();
  
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  delay(10);
  Serial.println("\n\n\n\n");
  Serial.println("setting up...");
  tempReporter
    .SetBrokerUrl(mqtt_server)
    .SetUSer("hallondisp")
    .SetPass("disphallon")
    .SetTopic(TOPIC_SPACE "temperature")
    .SetId(mqtt_temp_id);

  doorReporter
    .SetBrokerUrl(mqtt_server)
    .SetUSer("hallondisp")
    .SetPass("disphallon")
    .SetTopic(TOPIC_SPACE "door")
    .SetId(mqtt_door_id);

  powerReporter
    .SetBrokerUrl(mqtt_server)
    .SetUSer("hallondisp")
    .SetPass("disphallon")
    .SetTopic(TOPIC_SPACE "power")
    .SetId(mqtt_power_id);

  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(SSID, password);
  Serial.println(SSID);
  Serial.println(password);
  verifyWifi();
  
  ledControl.SetBlue(false);
  ledControl.SetRed(false);
  ledControl.SetGreen(false);
    
  if(!tempReporter.connect() || !doorReporter.connect() || !powerReporter.connect())
  {
    Serial.println("Could not connect to broker. restarting...");
    reboot();
  }
  
  attachInterrupt(digitalPinToInterrupt(powerReporter.GetTickPin()), handleInterrupt, FALLING);
  
   if (!MDNS.begin(IOT_ID)) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  ArduinoOTA.begin();
  ledControl.SetGreen(true);
}

void handleTempReporter() {
  tempReporter.reconnectingLoop();
  //garageReporter.report("Door!");
  long t = (millis() - tempReportLastSend) / 1000;
  if (t >= 10) {
    Serial.println("Sending temp report");
    tempReporter.Report();
    tempReportLastSend = millis();
  }
}

void handleDoorReporter() {
  doorReporter.reconnectingLoop();
  long t = (millis() - doorReportLastSend) / 1000;
  if (t >= 10) {
    Serial.println("Sending door report");
    bool state = doorReporter.Report();
    ledControl.SetRed(!state);
    doorReportLastSend = millis();
  }

}
void handlePowerReporter() {
  powerReporter.reconnectingLoop();
  powerReporter.Report();
}

void loop() {
  verifyWifi();
  handleTempReporter();  
  handleDoorReporter();
  handlePowerReporter();
  MDNS.update();
  ArduinoOTA.handle();
}
