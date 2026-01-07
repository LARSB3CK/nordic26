#include <WiFi.h>
#include <PubSubClient.h>

// ========= USER SETTINGS =========
const char* ssid     = "Stettin-Gestir";
const char* password = "12345abcde";

const char* mqtt_server = "test.mosquitto.org";
const int   mqtt_port   = 1883;

const char* mqtt_topic  = "frosti/b1/telemetry";
const char* device_id   = "frosti-b1";   // optional, but nice to include
// ================================

WiFiClient espClient;
PubSubClient client(espClient);

const int analogPin = A0; // Grove analog port A0

unsigned long lastPublish = 0;
unsigned long seq = 0;

void setup_wifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    // Random client ID to avoid collisions
    String clientId = "XIAO-ESP32C3-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected.");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2 seconds...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Grove Rotary -> MQTT test");

  analogReadResolution(12); // 0–4095

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  randomSeed(micros()); // for random client id
}

void loop() {
  // Ensure WiFi stays connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    setup_wifi();
  }

  // Ensure MQTT stays connected
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // Publish every 3 seconds
  unsigned long now = millis();
  if (now - lastPublish >= 3000) {
    lastPublish = now;

    // Read analog input
    int raw = analogRead(analogPin);         // 0–4095
    float percent = (raw / 4095.0) * 100.0;  // 0–100%
    float voltage = (raw / 4095.0) * 3.3;    // approx

    // Create JSON payload
    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"device\":\"%s\",\"seq\":%lu,\"raw\":%d,\"percent\":%.1f,\"voltage\":%.2f}",
             device_id, seq++, raw, percent, voltage);

    // Publish message
    bool ok = client.publish(mqtt_topic, payload);

    // Serial debug output
    Serial.print("Published to ");
    Serial.print(mqtt_topic);
    Serial.print(" : ");
    Serial.print(payload);
    Serial.print("  [");
    Serial.print(ok ? "OK" : "FAILED");
    Serial.println("]");
  }
}
