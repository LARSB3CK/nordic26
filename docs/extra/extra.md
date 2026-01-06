# Extra projects

## MQTT

Frosti explored the MQTT protocol using a simple ESP32C3 demo setup.

For initial testing on Windows, [MQTTX](https://mqttx.app/) can be used.

**Todo: Add MQTTX connection info**

The public test broker used is: [test.mosquitto.org](https://test.mosquitto.org/)

Xiao ESP32C3 code can be found here: [frosti](/docs/extra/frosti.ino)

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

