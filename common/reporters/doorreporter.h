#include<Arduino.h>
#include <mqttreporter.h>

class DoorReporter : public MqttReporter {
    public:
        //TempReporter(byte pin, mqttConfig mqtt_config, WiFiClient* espClient) 
        DoorReporter(byte pin, String id) 
          : MqttReporter(), pin_(pin), id_(id) 
        { 
          pinMode(pin_, INPUT);           
        }

        bool Report() {
          bool state = digitalRead(pin_);
          report("{ \"id\" : \"" + id_ + "\", \"door\" : " + (state ? "true" : "false") + "}");
          return state;
        }

    private:
        byte pin_;
        String id_;
};