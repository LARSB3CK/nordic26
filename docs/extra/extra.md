# Extra projects

## MQTT

Frosti explored the MQTT protocol using a simple ESP32C3 demo setup.

For initial testing on Windows, [MQTTX](https://mqttx.app/) can be used.

**Todo: Add MQTTX connection info**

The public test broker used is: [test.mosquitto.org](https://test.mosquitto.org/)

Xiao ESP32C3 code can be found here: [frosti](../extra/frosti/frosti.ino)

### What the ESP32 code does

The ESP32C3 connects to WiFi, then connects to the MQTT broker and starts publishing plain text messages.  
It also listens for incoming messages and prints them to the Serial monitor.

### Features

- Connects to WiFi and reconnects automatically if the connection drops
- Connects to the MQTT broker and reconnects automatically if MQTT disconnects
- Publishes a random status string to:
  - `frosti/status` every 10 seconds
- Subscribes to:
  - `frosti/esp` and prints received payloads to Serial

### Terminal commands on Linux

To listen for messages:

```
mosquitto_sub -h test.mosquitto.org -p 1883 -V mqttv311 -t 'frosti/#' -v -d
```

To send a command:
```
mosquitto_pub -h test.mosquitto.org -p 1883 -V mqttv311   -t 'frosti/esp' -m '{"cmd":"blink","n":3}'
```

## MQTT with some Grove stuff (Seeed studio kit)

Frosti now wanted to test out the massive kit he bought. We started by connected a variable resistor to it. Connection is simple, just use the included wires to join the sensor to the expansion board. 

ChatGPT then generated the code to get the readings, it's just analog reading from pin A0. 

Once that was working, we again connected to the test.mesquitto.org server and published the data. 

Using this command on the terminal (linux) we recieved the data fine. 

```
mosquitto_sub -h test.mosquitto.org -p 1883 -t "frosti/b1/telemetry" -v
```

we recieved the data just fine: 

```
frosti/b1/telemetry {"device":"frosti-b1","seq":314,"raw":2507,"percent":61.2,"voltage":2.02}
frosti/b1/telemetry {"device":"frosti-b1","seq":315,"raw":2508,"percent":61.2,"voltage":2.02}
frosti/b1/telemetry {"device":"frosti-b1","seq":0,"raw":2506,"percent":61.2,"voltage":2.02}
frosti/b1/telemetry {"device":"frosti-b1","seq":1,"raw":2511,"percent":61.3,"voltage":2.02}
frosti/b1/telemetry {"device":"frosti-b1","seq":2,"raw":1593,"percent":38.9,"voltage":1.28}
frosti/b1/telemetry {"device":"frosti-b1","seq":3,"raw":4095,"percent":100.0,"voltage":3.30}
frosti/b1/telemetry {"device":"frosti-b1","seq":4,"raw":221,"percent":5.4,"voltage":0.18}
```

Next we included the built in OLED on the Grove expansion board! 

**Todo: Add picture**
