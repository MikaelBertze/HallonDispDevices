#ifndef _MQTT_REPORTER_H
#define _MQTT_REPORTER_H

#include <Arduino.h>
#include <PubSubClient.h>


#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "This ain't a ESP8266 or ESP32!"
#endif

#define MSG_BUFFER_SIZE 100

struct mqttConfig {
    const char* broker;
    const char* topic;
    const char* id;
    const char* user;
    const char* pass;
};

class MqttReporter {
  public:
    bool connect() {
      // Attempt to connect
      client_.setServer(config_.broker, 1883);
      if (client_.connect(config_.id, config_.user, config_.pass)) {
         Serial.println("connected to broker");
         client_.publish("/megatron/IoT_status", "CONNECTED!");
         return true;
      } 
      else {
         Serial.print("failed, rc=");
         Serial.print(client_.state());
         return false;
      }
    }

    void reconnectingLoop() {
      if (!client_.loop()) {
        if (!connect());
          ESP.restart();
      }
    }

    MqttReporter& SetBrokerUrl(const char* url) {
      config_.broker = url;
      return *this;
    }
    
    MqttReporter& SetTopic(const char* topic) {
      config_.topic = topic;
      return *this;
    }

    MqttReporter& SetId(const char* id) {
      config_.id = id;
      return *this;
    }

    MqttReporter& SetUSer(const char* user) {
      config_.user = user;
      return *this;
    }
    MqttReporter& SetPass(const char* pass) {
      config_.pass = pass;
      return *this;
    }
  
  protected:
    MqttReporter()
      : espClient_() 
    { 
      client_ = PubSubClient(espClient_);
    }
    
    void report(String message) {
      char msg[MSG_BUFFER_SIZE];
      snprintf (msg, MSG_BUFFER_SIZE, message.c_str());
      client_.publish(config_.topic, msg);
    }

  private:
    mqttConfig config_;
    WiFiClient espClient_;
    PubSubClient client_;
    
};

#endif