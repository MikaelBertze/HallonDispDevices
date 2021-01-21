#include<Arduino.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <mqttreporter.h>

class TempReporter : public MqttReporter {
    public:
        //TempReporter(byte pin, mqttConfig mqtt_config, WiFiClient* espClient) 
        TempReporter(byte pin, String id) 
          : MqttReporter(), id_(id), oneWire_(pin), tempSensor_(&oneWire_) 
        {            
        }

        void Report() {
          tempSensor_.requestTemperatures();
          float temp = tempSensor_.getTempCByIndex(0);
          Serial.println(temp);
          report("{ \"id\" : \"" + id_ + "\", \"temp\" : " + String(temp,1) + " }");
        }

    private:
        String id_;
        OneWire oneWire_;
        DallasTemperature tempSensor_; 
};