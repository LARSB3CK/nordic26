#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <U8g2lib.h>

// ========= USER SETTINGS =========
const char* ssid     = "Stettin-Gestir";
const char* password = "12345abcde";

// Try IPv4 first to avoid DNS/IPv6 issues on some networks
const char* mqtt_server = "test.mosquitto.org";
const int   mqtt_port   = 1883;

const char* mqtt_topic  = "frosti/b1/telemetry";
const char* device_id   = "frosti-b1";
// ================================

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// OLED (XIAO Expansion Board)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Rotary sensor
const int analogPin = A0;

// Timers
unsigned long lastPublish = 0;
unsigned long lastOLED = 0;
unsigned long seq = 0;

// Last readings (for OLED refresh)
int lastRaw = 0;
float lastPercent = 0;
float lastVoltage = 0;

// Show brief status message on OLED (helper)
void oledSplash(const char* msg1, const char* msg2 = "") {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 14, msg1);
  u8g2.drawStr(0, 30, msg2);
  u8g2.sendBuffer();
}

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
    if (millis() - start > 20000) {  // 20s timeout
      Serial.println("\nWiFi connect timeout, restarting WiFi...");
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
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2s...");
      oledSplash("MQTT failed", "Retrying...");
      delay(2000);
    }
  }
}

void updateReadings() {
  lastRaw = analogRead(analogPin);
  lastPercent = (lastRaw / 4095.0) * 100.0;
  lastVoltage = (lastRaw / 4095.0) * 3.3;
}

void updateOLED() {
  // Status fields
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool mqttOk = client.connected();
  int rssi = wifiOk ? WiFi.RSSI() : 0;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);

  // Header
  u8g2.drawStr(0, 12, "Frosti Buoy B1");

  // WiFi + MQTT status
  char line0[32];
  snprintf(line0, sizeof(line0), "WiFi:%s RSSI:%d",
           wifiOk ? "OK" : "NO", wifiOk ? rssi : 0);
  u8g2.drawStr(0, 26, line0);

  char line1[32];
  snprintf(line1, sizeof(line1), "MQTT:%s Seq:%lu",
           mqttOk ? "OK" : "NO", seq);
  u8g2.drawStr(0, 38, line1);

  // Sensor values
  char line2[32];
  snprintf(line2, sizeof(line2), "Raw:%d  %.1f%%", lastRaw, lastPercent);
  u8g2.drawStr(0, 52, line2);

  char line3[32];
  snprintf(line3, sizeof(line3), "V: %.2fV", lastVoltage);
  u8g2.drawStr(0, 64, line3);

  u8g2.sendBuffer();
}

void publishMQTT() {
  char payload[160];
  snprintf(payload, sizeof(payload),
           "{\"device\":\"%s\",\"seq\":%lu,\"raw\":%d,\"percent\":%.1f,\"voltage\":%.2f}",
           device_id, seq, lastRaw, lastPercent, lastVoltage);

  bool ok = client.publish(mqtt_topic, payload);

  Serial.print("Published to ");
  Serial.print(mqtt_topic);
  Serial.print(" : ");
  Serial.print(payload);
  Serial.print("  [");
  Serial.print(ok ? "OK" : "FAILED");
  Serial.println("]");

  seq++;
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("Starting Frosti B1: OLED + MQTT + Rotary");

  analogReadResolution(12);
  randomSeed(micros());

  // Init I2C + OLED
  Wire.begin();
  u8g2.begin();

  oledSplash("Frosti Buoy B1", "Booting...");
  delay(800);

  setup_wifi();

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setSocketTimeout(10); // seconds
  client.setKeepAlive(30);     // seconds
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

  // Update sensor readings frequently (so OLED looks responsive)
  updateReadings();

  unsigned long now = millis();

  // OLED refresh ~5 times per second (every 200ms)
  if (now - lastOLED >= 200) {
    lastOLED = now;
    updateOLED();
  }

  // Publish every 3 seconds
  if (now - lastPublish >= 3000) {
    lastPublish = now;
    publishMQTT();
  }
}
