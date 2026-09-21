#include <WiFi.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "TFT_22_ILI9225.h"
#include "SimpleDHT.h"
#include "PubSubClient.h"

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "mdow/class305/data";

const uint8_t TFT_SCK = 18;
const uint8_t TFT_MISO = 19;
const uint8_t TFT_MOSI = 23;
const uint8_t TFT_CS = 5;
const uint8_t TFT_RST = 26;
const uint8_t TFT_DC = 27;
const uint8_t TFT_LED = 25;
const uint8_t DHT_PIN = 14;
const uint8_t LIGHT_PIN = 33;

const uint16_t APP_BACKGROUND = 0x1082;
const uint16_t APP_PANEL = 0x2128;
const uint16_t APP_EDGE = 0x4208;
const uint16_t APP_HEADER = 0x18C3;
const uint16_t APP_RED = 0xF800;
const uint16_t APP_CYAN = 0x07FF;
const uint16_t APP_YELLOW = 0xFFE0;
const uint16_t APP_ORANGE = 0xFD20;
const uint16_t APP_GREEN = 0x07E0;
const uint16_t APP_WHITE = 0xFFFF;
const uint16_t APP_MUTED = 0xBDF7;
const uint16_t SCREEN_WIDTH = 176;
const uint16_t SCREEN_HEIGHT = 220;

const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long MQTT_INTERVAL = 10000;
const unsigned long ANIMATION_INTERVAL = 220;

TFT_22_ILI9225 ili9225(TFT_RST, TFT_DC, TFT_CS, TFT_LED);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
SimpleDHT11 dht11(DHT_PIN);

class AdafruitILI9225 : public Adafruit_GFX {
public:
  explicit AdafruitILI9225(TFT_22_ILI9225& display) : Adafruit_GFX(SCREEN_WIDTH, SCREEN_HEIGHT), display(display) {}

  void begin() {
    display.begin();
    display.setOrientation(0);
    display.setBacklight(true);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x >= 0 && x < width() && y >= 0 && y < height()) {
      display.drawPixel((uint16_t)x, (uint16_t)y, color);
    }
  }

  void fillScreen(uint16_t color) override {
    display.fillRectangle(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, color);
  }

  void fillRect(int16_t x, int16_t y, int16_t rectWidth, int16_t rectHeight, uint16_t color) override {
    if (rectWidth <= 0 || rectHeight <= 0) return;
    int16_t right = x + rectWidth - 1;
    int16_t bottom = y + rectHeight - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right >= width()) right = width() - 1;
    if (bottom >= height()) bottom = height() - 1;
    if (x <= right && y <= bottom) {
      display.fillRectangle((uint16_t)x, (uint16_t)y, (uint16_t)right, (uint16_t)bottom, color);
    }
  }

private:
  TFT_22_ILI9225& display;
};

AdafruitILI9225 screen(ili9225);

byte temperature = 0;
byte humidity = 0;
int brightness = 0;
int dhtError = SimpleDHTErrSuccess;
bool wifiConnected = false;
bool mqttConnected = false;
String connectionMessage = "STARTING";
unsigned long lastSensorRead = 0;
unsigned long lastMqttPublish = 0;
unsigned long lastAnimation = 0;
uint8_t animationStep = 0;

void drawThermometerIcon(int16_t x, int16_t y, uint16_t color) {
  screen.drawRoundRect(x + 6, y, 9, 26, 4, color);
  screen.fillCircle(x + 10, y + 30, 8, color);
  screen.fillRect(x + 8, y + 10, 5, 21, color);
}

void drawDropIcon(int16_t x, int16_t y, uint16_t color) {
  screen.fillTriangle(x + 10, y, x, y + 18, x + 20, y + 18, color);
  screen.fillCircle(x + 10, y + 18, 10, color);
  screen.fillCircle(x + 10, y + 18, 5, APP_PANEL);
}

void drawSunIcon(int16_t x, int16_t y, uint16_t color) {
  screen.fillCircle(x + 10, y + 10, 6, color);
  screen.drawLine(x + 10, y, x + 10, y + 20, color);
  screen.drawLine(x, y + 10, x + 20, y + 10, color);
  screen.drawLine(x + 3, y + 3, x + 17, y + 17, color);
  screen.drawLine(x + 17, y + 3, x + 3, y + 17, color);
}

void drawStatusBar() {
  screen.fillRect(0, 0, SCREEN_WIDTH, 17, APP_HEADER);
  screen.setTextSize(1);
  screen.setTextColor(wifiConnected ? APP_GREEN : APP_ORANGE);
  screen.setCursor(5, 4);
  screen.print("WIFI:");
  screen.print(wifiConnected ? "O" : "X");
  screen.setTextColor(mqttConnected ? APP_GREEN : APP_ORANGE);
  screen.setCursor(67, 4);
  screen.print("MQTT:");
  screen.print(mqttConnected ? "O" : "X");
}

void drawConnectionScreen() {
  screen.fillScreen(APP_BACKGROUND);
  drawStatusBar();
  screen.fillRect(0, 17, SCREEN_WIDTH, 30, APP_HEADER);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(17, 24);
  screen.print("ENV MONITOR");
  screen.setTextSize(1);
  screen.setTextColor(APP_CYAN);
  screen.setCursor(30, 105);
  screen.print(connectionMessage);
}

void drawStaticLayout() {
  screen.fillScreen(APP_BACKGROUND);
  drawStatusBar();
  screen.fillRect(0, 17, SCREEN_WIDTH, 30, APP_HEADER);
  screen.fillRect(0, 44, SCREEN_WIDTH, 3, APP_CYAN);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(17, 24);
  screen.print("ENV MONITOR");

  screen.fillRoundRect(7, 53, 162, 52, 6, APP_PANEL);
  screen.drawRoundRect(7, 53, 162, 52, 6, APP_RED);
  drawThermometerIcon(18, 64, APP_RED);
  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(51, 62);
  screen.print("TEMPERATURE");

  screen.fillRoundRect(7, 112, 80, 82, 6, APP_PANEL);
  screen.drawRoundRect(7, 112, 80, 82, 6, APP_CYAN);
  drawDropIcon(17, 122, APP_CYAN);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(17, 151);
  screen.print("HUMIDITY");

  screen.fillRoundRect(89, 112, 80, 82, 6, APP_PANEL);
  screen.drawRoundRect(89, 112, 80, 82, 6, APP_YELLOW);
  drawSunIcon(108, 121, APP_YELLOW);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(101, 151);
  screen.print("LIGHT");

  screen.setTextColor(APP_MUTED);
  screen.setCursor(8, 204);
  screen.print("MQTT SENSOR DATA");
}

void drawSensorValues() {
  screen.fillRect(51, 75, 108, 25, APP_PANEL);
  screen.setTextSize(3);
  if (dhtError == SimpleDHTErrSuccess) {
    screen.setTextColor(APP_WHITE);
    screen.setCursor(53, 75);
    screen.print(temperature);
    screen.print(" C");
  } else {
    screen.setTextColor(APP_ORANGE);
    screen.setCursor(53, 78);
    screen.print("ERROR");
  }

  screen.fillRect(13, 163, 68, 25, APP_PANEL);
  screen.setTextSize(2);
  if (dhtError == SimpleDHTErrSuccess) {
    screen.setTextColor(APP_WHITE);
    screen.setCursor(19, 165);
    screen.print(humidity);
    screen.print("%");
  } else {
    screen.setTextColor(APP_ORANGE);
    screen.setCursor(18, 169);
    screen.print("ERR");
  }

  screen.fillRect(95, 163, 68, 25, APP_PANEL);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(103, 165);
  screen.print(brightness);
  screen.print("%");
  screen.fillRect(98, 181, 62, 8, APP_PANEL);
  screen.drawRoundRect(98, 181, 62, 8, 2, APP_EDGE);
  uint16_t barWidth = ((uint32_t)56 * brightness) / 100;
  if (barWidth > 0) screen.fillRoundRect(101, 183, barWidth, 4, 1, APP_YELLOW);
}

void updateAnimation() {
  screen.fillRect(143, 201, 27, 18, APP_BACKGROUND);
  const int16_t positions[] = {148, 157, 166};
  for (uint8_t index = 0; index < 3; index++) {
    uint16_t color = index == animationStep ? APP_CYAN : APP_EDGE;
    screen.fillCircle(positions[index], 210, index == animationStep ? 3 : 2, color);
  }
  animationStep = (animationStep + 1) % 3;
}

void updateStatusAndFooter() {
  drawStatusBar();
  screen.fillRect(8, 201, 132, 18, APP_BACKGROUND);
  screen.setTextSize(1);
  screen.setCursor(8, 204);
  screen.setTextColor(mqttConnected ? APP_GREEN : APP_ORANGE);
  screen.print(mqttConnected ? "MQTT DATA READY" : "MQTT OFFLINE");
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  int lightRaw = analogRead(LIGHT_PIN);
  brightness = map(lightRaw, 4095, 0, 0, 100);
  brightness = constrain(brightness, 0, 100);
  drawSensorValues();
  updateStatusAndFooter();
}

void connectWiFi() {
  wifiConnected = false;
  mqttConnected = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  connectionMessage = "WIFI CONNECTING";
  drawConnectionScreen();
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) {
    delay(500);
    connectionMessage = "WIFI CONNECTING...";
    drawConnectionScreen();
  }
  wifiConnected = WiFi.status() == WL_CONNECTED;
  connectionMessage = wifiConnected ? "WIFI CONNECTED" : "WIFI ERROR";
  drawConnectionScreen();
  delay(800);
}

void connectMQTT() {
  mqttConnected = false;
  if (!wifiConnected) return;
  connectionMessage = "MQTT CONNECTING";
  drawConnectionScreen();
  uint64_t chipId = ESP.getEfuseMac();
  char clientId[25];
  snprintf(clientId, sizeof(clientId), "esp32-%04X%08X", (unsigned int)(chipId >> 32), (unsigned int)chipId);
  if (mqttClient.connect(clientId)) {
    mqttConnected = true;
    connectionMessage = "MQTT CONNECTED";
  } else {
    connectionMessage = "MQTT ERROR";
  }
  drawConnectionScreen();
  delay(800);
}

void publishData() {
  if (!mqttConnected || dhtError != SimpleDHTErrSuccess) return;
  String payload = String("{\"temp\":") + temperature + ",\"humi\":" + humidity + ",\"light\":" + brightness + "}";
  if (mqttClient.publish(MQTT_TOPIC, payload.c_str())) {
    Serial.println(payload);
  } else {
    mqttConnected = false;
  }
  updateStatusAndFooter();
}

void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  screen.begin();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  drawConnectionScreen();
  connectWiFi();
  connectMQTT();
  drawStaticLayout();
  readSensors();
  publishData();
  lastSensorRead = millis();
  lastMqttPublish = millis();
  lastAnimation = millis();
}

void loop() {
  unsigned long now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    connectMQTT();
    drawStaticLayout();
    readSensors();
  }
  if (!mqttClient.connected() && wifiConnected) {
    connectMQTT();
    updateStatusAndFooter();
  }
  mqttClient.loop();
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    lastSensorRead = now;
  }
  if (now - lastMqttPublish >= MQTT_INTERVAL) {
    publishData();
    lastMqttPublish = now;
  }
  if (now - lastAnimation >= ANIMATION_INTERVAL) {
    updateAnimation();
    lastAnimation = now;
  }
}
