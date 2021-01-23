#include<Arduino.h>
#include <mqttreporter.h>

class PowerReporter : public MqttReporter {
    public:
        PowerReporter(byte pin, String id) 
          : MqttReporter(), pin_(pin), id_(id) 
        {

        }

        void SetTickPeriod(long tickPeriod){
          currentTickPeriod = tickPeriod;
          newData = true;
        }

        void Report() {
          if (currentTickPeriod != lastSentTickPeriod) {
            // each tick correspond to 1Wh consumed
            // calculating mean effect for this period
            // tickperiod [ms] => effect = 3600 * 1000 / tickperiod
            float mean_effect = (float)3600000 / currentTickPeriod;

            report("{ \"id\" : \"" + id_ + "\", \"mean_effect\" : " + String(mean_effect, 1) + ", \"power_tick_period\" : " + currentTickPeriod + " }");
            lastSentTickPeriod = currentTickPeriod;
          }
        }

        byte GetTickPin() {
          return pin_;
        }

    private:
        byte pin_;
        String id_;
        int currentTickPeriod;
        int lastSentTickPeriod;
        bool newData = false;
};