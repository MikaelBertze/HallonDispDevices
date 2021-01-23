#include<Arduino.h>
#include <mqttreporter.h>

class DoorReporter : public MqttReporter {
    public:
        DoorReporter(byte *doorpins, String *doorNames, byte numDoors)
          : MqttReporter(), pins_(doorpins), doorNames_(doorNames), numDoors_(numDoors)
        { 
          for (byte i = 0; i < numDoors; i++) {
            if (pins_[i] >= 0)
              pinMode(pins_[i], INPUT);
          }     
        }

        bool Report() {
          bool state = false;
          String message = "[ ";
          bool first = true;
          for (byte i = 0; i < numDoors_; i++) {
            if (pins_[i] >= 0) {
              if (!first)
                message += ", ";
              else
                first = false;
              state = digitalRead(pins_[i]);
              message += "{ \"id\" : \"" + doorNames_[i] + "\", \"door\" : " + (state ? "true" : "false") + "}";
            }
          }
          message += " ]";
          report(message);
          return state;
        }

    private:
        byte *pins_;
        String *doorNames_;
        byte numDoors_;
};