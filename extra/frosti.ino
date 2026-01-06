/*
  XIAO ESP32C3 MQTT over TLS Demo (test.mosquitto.org)
  - Publishes:
      frosti-demo-2026/status every 60s (random string) as JSON
      frosti-demo-2026/data   every 30s (random number 0-100) as JSON
  - Subscribes:
      frosti-demo-2026/esp -> prints received payload to Serial
  - Presence / LWT:
      frosti-demo-2026/presence: {"state":"online"} / {"state":"offline"}
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>

// -------------------- WiFi --------------------
const char* WIFI_SSID = "Stettin-Gestir";
const char* WIFI_PASS = "12345abcde";

// -------------------- MQTT over TLS (no auth) --------------------
const char* MQTT_HOST = "test.mosquitto.org";
const uint16_t MQTT_PORT = 8883;   // Encrypted, unauthenticated

// -------------------- Topic hierarchy --------------------
// Use a unique prefix on public broker to avoid collisions
const char* TOPIC_STATUS   = "frosti/status";
const char* TOPIC_DATA     = "frosti/data";
const char* TOPIC_COMMAND  = "frosti/esp";
const char* TOPIC_PRESENCE = "frosti/presence";

// -------------------- Timing --------------------
const unsigned long STATUS_INTERVAL_MS = 60UL * 1000UL; // 60 seconds
const unsigned long DATA_INTERVAL_MS   = 30UL * 1000UL; // 30 seconds

unsigned long lastStatusMs = 0;
unsigned long lastDataMs   = 0;

// -------------------- Random message list --------------------
const char* statusMessages[] = {
  "Halló, ég heiti Frosti!",
  "Frosti er flottur!",
  "Hvar er Jón Þór?",
  "Er kominn drekkutími",
  "Hvar er nammið?",
  "Frosti! Frosti! Frosti!",
  "Frostmannaeyjar",
  "Frosta í frambið!",
  "Friðarverlaun Frosta",
  "Allir að knúsa Frosta!"
};
const int STATUS_MSG_COUNT = sizeof(statusMessages) / sizeof(statusMessages[0]);

// -------------------- MQTT + TLS Client --------------------
WiFiClientSecure net;
MQTTClient mqtt(1024); // MQTT receive buffer size

bool mqttConnected = false;

// -------------------- Helpers --------------------
String deviceId;

// Forward declarations
void connectWiFi();
void connectMQTT();
void mqttMessageReceived(String &topic, String &payload);
void publishPresence(const char* state);
void publishStatus();
void publishData();

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  // Unique-ish client id
  deviceId = "frosti-" + String((uint32_t)ESP.getEfuseMac(), HEX);

  Serial.println();
  Serial.println("=== Frosti ESP32C3 MQTT over TLS ===");
  Serial.print("Device ID: ");
  Serial.println(deviceId);

  // Random seed (better than nothing)
  randomSeed(esp_random());

  connectWiFi();
  connectMQTT();
}

// -------------------- Loop --------------------
void loop() {
  // Keep MQTT alive and process incoming messages
  if (mqttConnected) {
    mqtt.loop();
  }

  // Reconnect logic if WiFi drops
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Lost connection. Reconnecting...");
    connectWiFi();
  }

  // Reconnect logic if MQTT drops
  if (!mqttConnected) {
    static unsigned long lastTry = 0;
    if (millis() - lastTry > 3000) {
      lastTry = millis();
      Serial.println("[MQTT] Reconnecting...");
      connectMQTT();
    }
  }

  // Publish at intervals
  unsigned long now = millis();

  if (mqttConnected && (now - lastStatusMs >= STATUS_INTERVAL_MS)) {
    lastStatusMs = now;
    publishStatus();
  }

  if (mqttConnected && (now - lastDataMs >= DATA_INTERVAL_MS)) {
    lastDataMs = now;
    publishData();
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

// -------------------- MQTT Connect (TLS) --------------------
void connectMQTT() {
  // If WiFi not connected, skip
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.println("[MQTT] Connecting to test.mosquitto.org over TLS (8883)...");

  // Simple testing mode: disable certificate validation.
  // Later you can replace this with net.setCACert(...)
  net.setInsecure();

  mqtt.begin(MQTT_HOST, MQTT_PORT, net);
  mqtt.onMessage(mqttMessageReceived);

  // Last Will and Testament
  StaticJsonDocument<128> lwtDoc;
  lwtDoc["device"] = deviceId;
  lwtDoc["state"]  = "offline";
  char lwtPayload[128];
  serializeJson(lwtDoc, lwtPayload);

  mqtt.setWill(TOPIC_PRESENCE, lwtPayload, true /* retained */, 0 /* qos */);

  Serial.print("[MQTT] Connecting as ");
  Serial.println(deviceId);

  // Connect without username/password
  bool ok = mqtt.connect(deviceId.c_str());

  if (!ok) {
    Serial.print("[MQTT] Connect failed. Error: ");
    Serial.println(mqtt.lastError());
    mqttConnected = false;
    return;
  }

  mqttConnected = true;
  Serial.println("[MQTT] Connected ✅");

  // Subscribe to command topic
  mqtt.subscribe(TOPIC_COMMAND, 0);
  Serial.print("[MQTT] Subscribed to ");
  Serial.println(TOPIC_COMMAND);

  // Publish presence online (retained)
  publishPresence("online");

  // Publish a boot message immediately
  publishStatus();
  publishData();
}

// -------------------- Publish Presence --------------------
void publishPresence(const char* state) {
  if (!mqttConnected) return;

  StaticJsonDocument<128> doc;
  doc["device"] = deviceId;
  doc["state"]  = state;
  doc["rssi"]   = WiFi.RSSI();

  char buf[128];
  serializeJson(doc, buf);

  mqtt.publish(TOPIC_PRESENCE, buf, true /* retained */, 0 /* qos */);

  Serial.print("[PUB] ");
  Serial.print(TOPIC_PRESENCE);
  Serial.print(" -> ");
  Serial.println(buf);
}

// -------------------- Publish Status --------------------
void publishStatus() {
  if (!mqttConnected) return;

  int idx = random(0, STATUS_MSG_COUNT);

  StaticJsonDocument<256> doc;
  doc["device"] = deviceId;
  doc["type"]   = "status";
  doc["msg"]    = statusMessages[idx];
  doc["rssi"]   = WiFi.RSSI();
  doc["uptime_s"] = (millis() / 1000);

  char buf[256];
  serializeJson(doc, buf);

  mqtt.publish(TOPIC_STATUS, buf, false /* retained */, 0 /* qos */);

  Serial.print("[PUB] ");
  Serial.print(TOPIC_STATUS);
  Serial.print(" -> ");
  Serial.println(buf);
}

// -------------------- Publish Data --------------------
void publishData() {
  if (!mqttConnected) return;

  int value = random(0, 101); // 0..100

  StaticJsonDocument<256> doc;
  doc["device"] = deviceId;
  doc["type"]   = "data";
  doc["value"]  = value;
  doc["unit"]   = "pct";
  doc["rssi"]   = WiFi.RSSI();
  doc["uptime_s"] = (millis() / 1000);

  char buf[256];
  serializeJson(doc, buf);

  mqtt.publish(TOPIC_DATA, buf, false /* retained */, 0 /* qos */);

  Serial.print("[PUB] ");
  Serial.print(TOPIC_DATA);
  Serial.print(" -> ");
  Serial.println(buf);
}

// -------------------- Incoming MQTT Message --------------------
void mqttMessageReceived(String &topic, String &payload) {
  Serial.println();
  Serial.print("[SUB] ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(payload);

  // Try to interpret payload:
  // 1) if it's JSON, parse and print nicely
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (!err) {
    Serial.println("[SUB] Payload is JSON:");
    serializeJsonPretty(doc, Serial);
    Serial.println();
    return;
  }

  // 2) if it's a number, print numeric
  bool isNumber = true;
  for (size_t i = 0; i < payload.length(); i++) {
    char c = payload[i];
    if (!(isDigit(c) || c == '.' || c == '-' || c == '+')) {
      isNumber = false;
      break;
    }
  }

  if (isNumber) {
    Serial.print("[SUB] Payload interpreted as number: ");
    Serial.println(payload.toFloat());
  } else {
    Serial.print("[SUB] Payload interpreted as string: ");
    Serial.println(payload);
  }
}
