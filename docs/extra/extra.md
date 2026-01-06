# Extra projects

## MQTT 

Frosti was interested in exploring the MQTT protocol. 

For initial testing on Windows, [MQTTX](https://mqttx.app/) can be used. 

**Todo: Add mqttx connection info**

This server is used for testing: [test.mosquitto.org](https://test.mosquitto.org/)

Xiao ESP32C3 code can be found here: [frosti](/docs/extra/frosti.ino) 

### What the ESP32 code does

The ESP32C3 connects to WiFi and then connects to an MQTT broker.  
It publishes small JSON messages regularly and listens for incoming commands.

### Features

- **Connects to WiFi** and keeps reconnecting automatically if WiFi drops
- **Connects to MQTT broker** and reconnects automatically if MQTT disconnects
- **Publishes JSON messages**:
  - `frosti/status` every 60 seconds (random status message)
  - `frosti/data` every 30 seconds (random value 0–100)
- **Subscribes to commands**:
  - Listens on `frosti/esp` and prints received messages to Serial
  - Attempts to parse JSON payloads, numbers, or plain text
- **Presence / LWT support**:
  - Publishes `frosti/presence` with `"online"` (retained)
  - Broker publishes `"offline"` automatically if device disconnects unexpectedly

### Terminal commands on Linux

To listen for messages:


```
mosquitto_sub -h test.mosquitto.org -p 1883 -V mqttv311 -t 'frosti/#' -v -d
```


To send a command:
```
mosquitto_pub -h test.mosquitto.org -p 1883 -V mqttv311   -t 'frosti/esp' -m '{"cmd":"blink","n":3}'
```

