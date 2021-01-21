# GarageThingy
This device is mounted in my garage. It reports the current power consumption, outdoor temperature and if the garage door is open or closed.

![circuit](circuit.png)

## Power consumption
The electricity meters installed by the power company usually have a IR-diode the blink when a fixed amount of Wh has been consumed. Mine is set to blink for each 1 Wh consumed. The blink is detected by a simple circuit with a foto transistor. The blink detector circuit is connected to one of the digital pins on the Wemos module. The state change on the connected input drives an interrupt that measures the time between two consecutive signals.

When a new blink has been detected an MQTT message with the time since the last signal is sent to the broker.

## Temperature
This device will read temperature from a DS20B18 temperature sensor and report the temperature value every 10s through MQTT messages.

## Door
A simple magnetic switch is mounted on the garage door. The switch is connected to one of the digital inputs. The current door state is sent every 10s through MQTT messages.
