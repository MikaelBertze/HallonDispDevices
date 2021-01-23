#include<Arduino.h>
#include <mqttreporter.h>
#include <datamodel.h>

class WaterReporter : public MqttReporter {
    public:
        WaterReporter(String id) 
          : MqttReporter(), id_(id) 
        {
          
        }

        void Report(data_model_t* datamodel) {
            String s = "{ \"id\" : \"water_thingy\" ";
            s += ", \"angle\" : [angle] ";
            s += ", \"diff\" : [diff] ";
            s += ", \"t_diff\" : [t_diff] ";
            s += ",\"consumption\" : [consumption] ";
            s += ", \"acc_consumption\" : [acc_consumption] ";
            s += "}";

            s.replace("[angle]", String(datamodel->state.angle, DEC));
            s.replace("[diff]", String(datamodel->state.angle_diff, DEC));
            s.replace("[t_diff]", String(datamodel->state.timediff_ms, DEC));
            s.replace("[consumption]", String(datamodel->state.consumption, DEC));
            s.replace("[acc_consumption]", String(datamodel->state.acc_consumption, DEC));

          report(s);
        }

    private:
        String id_;
};