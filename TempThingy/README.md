#TempThingy

This device will read temperature from a DS20B18 temperature sensor and report the temperature value through MQTT messages.

The temperature sensor is connected to the Wemos D1 pin. DS20B18 is a OneWire device so a pull-up is required on the data line.

![circuit](circuit.png)

## MQTT messages
The TempThingy will report periodically to a MQTT broker. The payload is a json string containg the temperature value (in celcius) and a configurable ID.

Example:
`{ "id" : "Garage", "temp": 12.5 }`

## Configure device
The device id is stored in the Wemos 
The device ID can be set through HTTP. Browse to http://device_ip/configure


