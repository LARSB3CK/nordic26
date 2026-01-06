/*
  Frosti ESP32C3 - MQTT Minimal Demo (Strings only)

  - Publishes:
      frosti/status every 10 seconds (random string)
  - Subscribes:
      frosti/esp -> prints received payload to Serial

  Broker:
      test.mosquitto.org :1883 (no TLS, no auth)
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <MQTTClient.h>

// -------------------- WiFi --------------------
const char* WIFI_SSID = "Stettin-Gestir";
const char* WIFI_PASS = "12345abcde";

// -------------------- MQTT --------------------
const char* MQTT_HOST = "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;

// -------------------- Topics --------------------
const char* TOPIC_STATUS  = "frosti/status";
const char* TOPIC_COMMAND = "frosti/esp";

// -------------------- Timing --------------------
const unsigned long PUBLISH_INTERVAL_MS = 10UL * 1000UL;
unsigned long lastPublishMs = 0;

// -------------------- Random message list --------------------
const char* statusMessages[] = {
  "hello from frosti",
  "mqtt is fun",
  "xiao esp32c3 online",
  "status: all good",
  "status: still running",
  "status: coffee time",
  "status: ping",
  "status: ok",
  "status: test message",
  "status: end"
};
const int STATUS_MSG_COUNT = sizeof(statusMessages) / sizeof(statusMessages[0]);

// -------------------- MQTT client --------------------
WiFiClient net;
MQTTClient mqtt(256);

bool mqttConnected = false;
String deviceId;

// Forward declarations
void connectWiFi();
void connectMQTT();
void mqttMessageReceived(String &topic, String &payload);

void setup() {
  Serial.begin(115200);
  delay(200);

  deviceId = "frosti-" + String((uint32_t)ESP.getEfuseMac(), HEX);

  Serial.println();
  Serial.println("=== Frosti ESP32C3 MQTT Minimal Demo ===");
  Serial.print("Device ID: ");
  Serial.println(deviceId);

  randomSeed(esp_random());

  connectWiFi();
  connectMQTT();
}

void loop() {
  // Maintain MQTT connection
  if (mqttConnected) {
    mqtt.loop();
  }

  // WiFi reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Lost connection. Reconnecting...");
    connectWiFi();
    mqttConnected = false;  // force MQTT reconnect after WiFi is back
  }

  // MQTT reconnect
  if (!mqttConnected) {
    static unsigned long lastTry = 0;
    if (millis() - lastTry > 3000) {
      lastTry = millis();
      Serial.println("[MQTT] Reconnecting...");
      connectMQTT();
    }
  }

  // Publish every interval
  unsigned long now = millis();
  if (mqttConnected && (now - lastPublishMs >= PUBLISH_INTERVAL_MS)) {
    lastPublishMs = now;

    int idx = random(0, STATUS_MSG_COUNT);
    const char* msg = statusMessages[idx];

    mqtt.publish(TOPIC_STATUS, msg);

    Serial.print("[PUB] ");
    Serial.print(TOPIC_STATUS);
    Serial.print(" -> ");
    Serial.println(msg);
  }

  delay(5);
}

// -------------------- WiFi Connect --------------------
void connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\n[WiFi] Timeout. Retrying...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      start = millis();
    }
  }

  Serial.println();
  Serial.print("[WiFi] Connected. IP: ");
  Serial.println(WiFi.localIP());
}

// -------------------- MQTT Connect --------------------
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;

  mqtt.begin(MQTT_HOST, MQTT_PORT, net);
  mqtt.onMessage(mqttMessageReceived);

  Serial.print("[MQTT] Connecting to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  bool ok = mqtt.connect(deviceId.c_str());

  if (!ok) {
    Serial.print("[MQTT] Connect failed. Error: ");
    Serial.println(mqtt.lastError());
    mqttConnected = false;
    return;
  }

  mqttConnected = true;
  Serial.println("[MQTT] Connected ✅");

  mqtt.subscribe(TOPIC_COMMAND);
  Serial.print("[MQTT] Subscribed to ");
  Serial.println(TOPIC_COMMAND);
}

// -------------------- Incoming MQTT Message --------------------
void mqttMessageReceived(String &topic, String &payload) {
  Serial.println();
  Serial.print("[SUB] ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(payload);
}
