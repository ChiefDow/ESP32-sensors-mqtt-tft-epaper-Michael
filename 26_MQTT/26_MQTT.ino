#include <WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>
#include <Wire.h>
#include <U8g2lib.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "mdow/class305/data";
const unsigned long MQTT_INTERVAL = 10000;

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;

SimpleDHT11 dht11(DHT_PIN);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

unsigned long lastPublish = 0;
unsigned long yellowLedOffAt = 0;
byte temperature = 0;
byte humidity = 0;
int lightRaw = 0;
int brightness = 0;
int dhtError = SimpleDHTErrSuccess;
String clientId;
String mqttStatus = "MQTT:WAIT";

void drawThermometer(int x, int y) {
  oled.drawFrame(x + 5, y, 5, 13);
  oled.drawDisc(x + 7, y + 15, 4);
  oled.drawBox(x + 6, y + 5, 3, 10);
  oled.drawLine(x + 10, y + 3, x + 12, y + 3);
  oled.drawLine(x + 10, y + 7, x + 12, y + 7);
  oled.drawLine(x + 10, y + 11, x + 12, y + 11);
}

void drawDrop(int x, int y) {
  oled.drawLine(x + 7, y, x + 2, y + 8);
  oled.drawLine(x + 7, y, x + 12, y + 8);
  oled.drawCircle(x + 7, y + 9, 5);
  oled.drawDisc(x + 7, y + 9, 3);
}

void drawSun(int x, int y) {
  oled.drawCircle(x + 7, y + 7, 4);
  oled.drawLine(x + 7, y, x + 7, y - 3);
  oled.drawLine(x + 7, y + 14, x + 7, y + 17);
  oled.drawLine(x, y + 7, x - 3, y + 7);
  oled.drawLine(x + 14, y + 7, x + 17, y + 7);
  oled.drawLine(x + 2, y + 2, x, y);
  oled.drawLine(x + 12, y + 2, x + 14, y);
  oled.drawLine(x + 2, y + 12, x, y + 14);
  oled.drawLine(x + 12, y + 12, x + 14, y + 14);
}

void drawProgressBar(int x, int y, int width, int height, int percent) {
  oled.drawFrame(x, y, width, height);
  int fillWidth = map(percent, 0, 100, 0, width - 2);
  if (fillWidth > 0) oled.drawBox(x + 1, y + 1, fillWidth, height - 2);
}

void drawScreen() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setFontPosTop();
  oled.drawFrame(0, 0, 64, 31);
  oled.drawFrame(64, 0, 64, 31);
  oled.drawFrame(0, 31, 128, 33);

  drawThermometer(3, 6);
  oled.setCursor(20, 4);
  oled.print("TEMP");
  oled.setCursor(20, 17);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(temperature);
    oled.print(" C");
  } else {
    oled.print("ERROR");
  }

  drawDrop(67, 6);
  oled.setCursor(84, 4);
  oled.print("HUM");
  oled.setCursor(84, 17);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(humidity);
    oled.print(" %");
  } else {
    oled.print("ERROR");
  }

  drawSun(3, 39);
  oled.setCursor(22, 36);
  oled.print("LIGHT");
  oled.setCursor(60, 36);
  oled.print(brightness);
  oled.print("%");
  drawProgressBar(86, 38, 38, 8, brightness);
  oled.setCursor(4, 53);
  oled.print(mqttStatus);
  oled.sendBuffer();
}

void updateLeds() {
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  bool mqttConnected = mqttClient.connected();
  digitalWrite(GREEN_LED_PIN, wifiConnected && mqttConnected ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, wifiConnected && !mqttConnected ? HIGH : LOW);
  if (millis() >= yellowLedOffAt) digitalWrite(YELLOW_LED_PIN, LOW);
}

void blinkPublishLed() {
  digitalWrite(YELLOW_LED_PIN, HIGH);
  yellowLedOffAt = millis() + 250;
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  lightRaw = analogRead(LIGHT_PIN);
  brightness = map(lightRaw, 4095, 0, 0, 100);
  brightness = constrain(brightness, 0, 100);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttStatus = "WIFI...";
  drawScreen();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
    mqttStatus = "WIFI OK";
  } else {
    Serial.println("Wi-Fi connection failed");
    mqttStatus = "WIFI ERR";
  }
  updateLeds();
  drawScreen();
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  mqttStatus = "MQTT...";
  updateLeds();
  drawScreen();

  Serial.print("Connecting to MQTT as ");
  Serial.println(clientId);
  if (mqttClient.connect(clientId.c_str())) {
    mqttStatus = "MQTT OK";
    Serial.println("MQTT connected");
  } else {
    mqttStatus = "MQTT ERR";
    Serial.print("MQTT connection failed, state=");
    Serial.println(mqttClient.state());
  }
  updateLeds();
  drawScreen();
}

void publishData() {
  if (!mqttClient.connected()) return;
  if (dhtError != SimpleDHTErrSuccess) {
    mqttStatus = "DHT ERR";
    Serial.print("DHT11 read failed, error=");
    Serial.println(dhtError);
    updateLeds();
    drawScreen();
    return;
  }

  String payload = String("{\"temp\":") + temperature +
                   ",\"humi\":" + humidity +
                   ",\"light\":" + brightness + "}";
  bool published = mqttClient.publish(MQTT_TOPIC, payload.c_str());
  mqttStatus = published ? "PUB OK" : "PUB ERR";
  Serial.print("MQTT topic: ");
  Serial.println(MQTT_TOPIC);
  Serial.print("MQTT payload: ");
  Serial.println(payload);
  if (published) blinkPublishLed();
  updateLeds();
  drawScreen();
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  Wire.begin(21, 22);
  oled.begin();
  oled.setFontPosTop();

  uint64_t chipId = ESP.getEfuseMac();
  char clientIdBuffer[24];
  snprintf(clientIdBuffer, sizeof(clientIdBuffer), "esp32-%04X%08X",
           static_cast<unsigned int>(chipId >> 32),
           static_cast<unsigned int>(chipId & 0xFFFFFFFF));
  clientId = clientIdBuffer;

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  readSensors();
  drawScreen();
  connectWiFi();
  connectMQTT();
  lastPublish = millis() - MQTT_INTERVAL;
}

void loop() {
  updateLeds();
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  readSensors();
  drawScreen();
  if (millis() - lastPublish >= MQTT_INTERVAL) {
    publishData();
    lastPublish = millis();
  }
  delay(1000);
}
