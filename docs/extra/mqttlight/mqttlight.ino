#include <WiFi.h>
#include <PubSubClient.h>     // Install: PubSubClient by Nick O'Leary
#include <Wire.h>
#include <U8g2lib.h>          // Install: U8g2 by olikraus
#include <ArduinoJson.h>      // Install: ArduinoJson by Benoit Blanchon

// ========= USER SETTINGS =========
const char* ssid     = "Stettin-Gestir";
const char* password = "12345abcde";

const char* mqtt_server = "test.mosquitto.org";
const int   mqtt_port   = 1883;

// We subscribe to the telemetry topic from the other XIAO:
const char* mqtt_sub_topic  = "frosti/b1/telemetry";

// Optional: publish our own status if you want later
// const char* mqtt_pub_topic = "frosti/b2/status";

const char* device_id = "frosti-b2";  // This device (the receiver)
// ================================

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// OLED (XIAO Expansion Board)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// LED (Grove LED Socket plugged into digital port)
const int ledPin = D0;     // Change to D1 / D2 / D3 if needed

// Threshold
const float THRESHOLD_PERCENT = 50.0;

// Timers
unsigned long lastOLED = 0;
unsigned long lastMsgMillis = 0;

// Last received data (shown on OLED)
float lastPercent = -1.0;
float lastVoltage = -1.0;
int lastRaw = -1;
unsigned long lastSeq = 0;

// LED state
bool ledOn = false;

// ---------- OLED helper ----------
void oledSplash(const char* msg1, const char* msg2 = "") {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 14, msg1);
  u8g2.drawStr(0, 30, msg2);
  u8g2.sendBuffer();
}

// ---------- WiFi ----------
void setup_wifi() {
  oledSplash("WiFi...", "Connecting");
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\nWiFi connect timeout, restarting...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      start = millis();
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  oledSplash("WiFi connected", WiFi.localIP().toString().c_str());
  delay(800);
}

// ---------- MQTT callback ----------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // payload is not null-terminated, so we must copy it into a char buffer
  static char msg[256];
  if (length >= sizeof(msg)) length = sizeof(msg) - 1;
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract fields (if present)
  if (doc.containsKey("percent")) lastPercent = doc["percent"].as<float>();
  if (doc.containsKey("voltage")) lastVoltage = doc["voltage"].as<float>();
  if (doc.containsKey("raw"))     lastRaw     = doc["raw"].as<int>();
  if (doc.containsKey("seq"))     lastSeq     = doc["seq"].as<unsigned long>();

  lastMsgMillis = millis();

  // Control LED based on percent
  bool shouldBeOn = (lastPercent >= 0 && lastPercent < THRESHOLD_PERCENT);
  ledOn = shouldBeOn;
  digitalWrite(ledPin, ledOn ? HIGH : LOW);

  Serial.print("Percent=");
  Serial.print(lastPercent, 1);
  Serial.print("% => LED ");
  Serial.println(ledOn ? "ON" : "OFF");
}

// ---------- MQTT reconnect ----------
void reconnect_mqtt() {
  oledSplash("MQTT...", "Connecting");
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print(" ... ");

    String clientId = "XIAO-ESP32C3-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected!");
      oledSplash("MQTT connected", mqtt_server);
      delay(500);

      // Subscribe to telemetry topic
      if (client.subscribe(mqtt_sub_topic)) {
        Serial.print("Subscribed to: ");
        Serial.println(mqtt_sub_topic);
      } else {
        Serial.print("Subscribe FAILED for: ");
        Serial.println(mqtt_sub_topic);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2s...");
      oledSplash("MQTT failed", "Retrying...");
      delay(2000);
    }
  }
}

// ---------- OLED update ----------
void updateOLED() {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool mqttOk = client.connected();
  int rssi = wifiOk ? WiFi.RSSI() : 0;

  // Optional: safety timeout - turn LED off if no data for 15s
  // This avoids LED being stuck ON if the sender dies.
  unsigned long age = millis() - lastMsgMillis;
  if (lastMsgMillis > 0 && age > 15000) {
    ledOn = false;
    digitalWrite(ledPin, LOW);
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);

  u8g2.drawStr(0, 12, "Frosti Buoy B2 (LED)");

  char line0[32];
  snprintf(line0, sizeof(line0), "WiFi:%s RSSI:%d", wifiOk ? "OK" : "NO", wifiOk ? rssi : 0);
  u8g2.drawStr(0, 26, line0);

  char line1[32];
  snprintf(line1, sizeof(line1), "MQTT:%s Sub: b1", mqttOk ? "OK" : "NO");
  u8g2.drawStr(0, 38, line1);

  char line2[32];
  if (lastPercent >= 0) {
    snprintf(line2, sizeof(line2), "Pct: %.1f%%  LED:%s", lastPercent, ledOn ? "ON" : "OFF");
  } else {
    snprintf(line2, sizeof(line2), "Pct: --.-%%  LED:%s", ledOn ? "ON" : "OFF");
  }
  u8g2.drawStr(0, 52, line2);

  char line3[32];
  if (lastMsgMillis == 0) {
    snprintf(line3, sizeof(line3), "No data yet");
  } else {
    snprintf(line3, sizeof(line3), "Seq:%lu Age:%lus", lastSeq, (millis() - lastMsgMillis) / 1000);
  }
  u8g2.drawStr(0, 64, line3);

  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("Starting Frosti B2: MQTT subscriber + LED + OLED");

  randomSeed(micros());

  // LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // OLED init
  Wire.begin();
  u8g2.begin();
  oledSplash("Frosti Buoy B2", "Booting...");
  delay(800);

  // WiFi
  setup_wifi();

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setSocketTimeout(10);
  client.setKeepAlive(30);
}

void loop() {
  // Ensure WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    setup_wifi();
  }

  // Ensure MQTT
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // OLED refresh every 200ms
  unsigned long now = millis();
  if (now - lastOLED >= 200) {
    lastOLED = now;
    updateOLED();
  }
}
