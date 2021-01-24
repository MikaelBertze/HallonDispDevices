#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>  
#include <ArduinoOTA.h>
#include <ledcontrol.h>
#include <tempreporter.h>
#include "credentials.h"

#ifndef IOT_ID
  #error “IOT_ID not specified. Define this in platform.ini for your target”
#endif 

volatile long lastDebounceTime = 0;
volatile long lastTick = -1;

TempReporter tempReporter(TEMP_PIN, TEMP_ENABLE, IOT_ID);

long tempReportLastSend = 0;

LedControl leds(D2, D3, D4, true);

void reboot() {
  leds.RebootSignal();
  ESP.restart();
}

void verifyWifi() {
  
  if (WiFi.status() == WL_CONNECTED)
    return;
  
  Serial.print("No connection...");
  leds.SetRed(true);
  byte num_retries = 60;
  byte retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("connecting...");
    delay(1000);
    leds.ToggleBlue();
    if(retries++ > num_retries){
      Serial.print("Restarting.");
      reboot();
    }
  }
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("ChipID: ");
  Serial.println(ESP.getChipId());
  leds.SetRed(false);
}

void setup() {
  Serial.begin(9600);
  //Serial.setDebugOutput(true);
  Serial.println("");
  Serial.println("setting up...");
  tempReporter
    .SetBrokerUrl(MQTT_broker)
    .SetTopic(TEMP_TOPIC)
    .SetId(IOT_ID)
    .SetUSer(MQTT_user)
    .SetPass(MQTT_password);

  leds.InitLeds();
  leds.ledTest();
  leds.SetBlue(true);
  leds.SetRed(true);
  leds.SetGreen(true);
  
  WiFi.mode(WIFI_STA); // Appears to be more stable in STA only mode
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(SSID, password);
  verifyWifi();
    
  for(byte i = 0; i < 10; i++){
    delay(50);
    leds.ToggleBlue();
    leds.ToggleRed();
    leds.ToggleGreen();
  }
  leds.SetBlue(false);
  leds.SetRed(false);
  leds.SetGreen(false);

  Serial.println(MQTT_broker);

  if(!tempReporter.connect())
  {
    Serial.println("Could not connect to broker. restarting...");
    reboot();
  }
  
  if (!MDNS.begin(IOT_ID)) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  ArduinoOTA.begin();

  leds.SetGreen(true);
}

void handleTempReporter() {
  tempReporter.reconnectingLoop();
  long t = (millis() - tempReportLastSend) / 1000;
  if (t >= 10) {
    Serial.println("Sending temp report");
    tempReporter.Report();

    tempReportLastSend = millis();
  }
}

void loop() {
  verifyWifi();
  handleTempReporter();  
  MDNS.update();
  ArduinoOTA.handle();
}
