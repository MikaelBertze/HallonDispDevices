#include<Arduino.h>
#include <mqttreporter.h>

class DoorReporter : public MqttReporter {
    public:
        //TempReporter(byte pin, mqttConfig mqtt_config, WiFiClient* espClient) 
        DoorReporter(byte *doorpins, String *doorNames, byte numDoors)
          : MqttReporter(), pins_(doorpins), doorNames_(doorNames), numDoors_(numDoors)
        { 
          for (byte i = 0; i < numDoors; i++) {
            //  pins_[i] = doorpins[i];
            //  doorNames_[i] = doorNames_[i];
            if (pins_[i] >= 0)
              pinMode(pins_[i], INPUT);
          }     
        }

        bool Report() {
          bool state = false;
          for (byte i = 0; i < numDoors_; i++) {
            if (pins_[i] >= 0) {
              state = digitalRead(pins_[i]);
              report("{ \"id\" : \"" + doorNames_[i] + "\", \"door\" : " + (state ? "true" : "false") + "}");
            }
          }
          return state;

        }

    private:
        byte *pins_;
        String *doorNames_;
        byte numDoors_;
};