#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

#define DHTPIN 4           // Pin connected to DHT11 sensor
#define DHTTYPE DHT11      // Sensor type
#define LED_PIN 5          // NeoPixel data pin
#define NUMPIXELS 8        // Number of NeoPixels

// WiFi credentials
const char* ssid     = "Test";
const char* password = "12345678";

// MQTT broker details (HiveMQ public broker)
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;
const char* mqtt_user   = nullptr;   // Not required for public broker
const char* mqtt_pass   = nullptr;   // Not required for public broker
const char* mqtt_clientID = "ESP32C3_TempLightCtrl";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

String mode = "auto";

void setup_wifi() {
  Serial.print("Connecting to WiFi ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);

  if (String(topic) == "home/room1/light/color") {
    mode = msg;
    applyColor();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_clientID)) {
      Serial.println("connected");
      client.subscribe("home/room1/light/color");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void applyColor() {
  float t = dht.readTemperature(); // Celsius
  if (isnan(t)) {
    Serial.println("Failed to read temperature!");
    return;
  }
  int r = 0, g = 0, b = 0;

  if (mode == "auto") {
    if (t < 18) {
      b = 255;    // Blue for cold
    }
    else if (t <= 28) {
      g = 255;    // Green for comfortable
    }
    else {
      r = 255;    // Red for hot
    }
  }
  else if (mode == "red") {
    r = 255;
  }
  else if (mode == "blue") {
    b = 255;
  }
  else if (mode == "green") {
    g = 255;
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  strip.begin();
  strip.show(); // Initialize all pixels off
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    char tempBuf[8];
    char humBuf[8];
    dtostrf(temperature, 4, 2, tempBuf);
    dtostrf(humidity, 4, 2, humBuf);

    client.publish("home/room1/temperature", tempBuf);
    client.publish("home/room1/humidity", humBuf);

    Serial.print("Published Temperature: ");
    Serial.print(tempBuf);
    Serial.print(" °C  |  Humidity: ");
    Serial.print(humBuf);
    Serial.println(" %");
  } 
  else {
    Serial.println("Failed to read from DHT sensor!");
  }

  applyColor();
  delay(2000);
}
