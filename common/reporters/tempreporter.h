#include<Arduino.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <mqttreporter.h>

class TempReporter : public MqttReporter {
    public:
        //TempReporter(byte pin, mqttConfig mqtt_config, WiFiClient* espClient) 
        TempReporter(byte pin, byte enable_pin, String id) 
          : MqttReporter(), id_(id), enablePin_(enable_pin), oneWire_(pin), tempSensor_(&oneWire_) 
        {
          pinMode(enablePin_, OUTPUT);
          digitalWrite(enable_pin, false);            
        }

        void Report() {

          digitalWrite(enablePin_, true);
          delay(1);
          tempSensor_.requestTemperatures();
          float temp = tempSensor_.getTempCByIndex(0);
          digitalWrite(enablePin_, false);
          Serial.println(temp);
          report("{ \"id\" : \"" + id_ + "\", \"temp\" : " + String(temp,1) + " }");

        }

    private:
        String id_;
        byte enablePin_;
        OneWire oneWire_;
        DallasTemperature tempSensor_; 
};