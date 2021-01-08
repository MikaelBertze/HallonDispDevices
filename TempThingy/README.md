# TempThingy

This device will read temperature from a DS20B18 temperature sensor and report the temperature value through MQTT messages.
The temperature sensor is connected to the Wemos D1 pin. DS20B18 is a OneWire device so a pull-up is required on the data line. Three leds are connected for some simple status indication.

![circuit](circuit.png)

## MQTT messages
The TempThingy will report periodically to a MQTT broker. The payload is a json string containg the temperature value (in celcius) and a configurable ID.

Example:
`{ "id" : "Garage", "temp": 12.5 }`

## Configure device
After first programming, the device will be configured to use the default id `tempthingy`. To set a new ID , browse to http://tempthingy.local/configure and follow the instructions. The device id is stored in the microcontroller EEPROM and will be persistant until a new configuration is done. 


