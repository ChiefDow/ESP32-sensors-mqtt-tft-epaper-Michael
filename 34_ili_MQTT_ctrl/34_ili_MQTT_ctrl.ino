#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "TFT_22_ILI9225.h"
#include "SimpleDHT.h"
#include "PubSubClient.h"
#include <ArduinoJson.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "mdow/class305/data";
const char* MQTT_CTRL_TOPIC = "mdow/class305/ctrl";
const char* MQTT_CTRL_WILDCARD = "mdow/class305/ctrl/#";
const char* GLED_CTRL_TOPIC = "mdow/class305/ctrl/gled";
const char* YLED_CTRL_TOPIC = "mdow/class305/ctrl/yled";
const char* RLED_CTRL_TOPIC = "mdow/class305/ctrl/rled";

const uint8_t TFT_SCK = 18;
const uint8_t TFT_MISO = 19;
const uint8_t TFT_MOSI = 23;
const uint8_t TFT_CS = 5;
const uint8_t TFT_RST = 26;
const uint8_t TFT_DC = 27;
const uint8_t TFT_LED = 25;
const uint8_t DHT_PIN = 14;
const uint8_t LIGHT_PIN = 33;
const uint8_t GREEN_LED_PIN = 15;
const uint8_t YELLOW_LED_PIN = 2;
const uint8_t RED_LED_PIN = 4;

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
const unsigned long CLOCK_INTERVAL = 10000;
const long TAIWAN_GMT_OFFSET = 8 * 3600;

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
String lastCommand = "NO COMMAND";
String currentDateTime = "--/-- --- --:--";
unsigned long lastClockUpdate = 0;

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
  screen.setCursor(8, 201);
  screen.setTextColor(mqttConnected ? APP_GREEN : APP_ORANGE);
  screen.print(lastCommand);
  screen.setCursor(8, 210);
  screen.setTextColor(APP_WHITE);
  screen.print(currentDateTime);
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  int lightRaw = analogRead(LIGHT_PIN);
  brightness = map(lightRaw, 4095, 0, 0, 100);
  brightness = constrain(brightness, 0, 100);
  drawSensorValues();
  updateStatusAndFooter();
}

void updateClock() {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo, 1000)) {
    char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%m/%d %a %H:%M", &timeInfo);
    currentDateTime = timeBuffer;
  } else {
    currentDateTime = "--/-- --- --:--";
  }
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

bool decodeState(JsonVariant value, bool& state) {
  if (value.is<bool>()) {
    state = value.as<bool>();
    return true;
  }
  if (value.is<int>()) {
    state = value.as<int>() != 0;
    return true;
  }
  if (value.is<const char*>()) {
    String text = value.as<const char*>();
    text.trim();
    text.toLowerCase();
    if (text == "on" || text == "true" || text == "1") {
      state = true;
      return true;
    }
    if (text == "off" || text == "false" || text == "0") {
      state = false;
      return true;
    }
  }
  return false;
}

void applyLedState(JsonVariant value, uint8_t pin, const char* label, String& result) {
  bool state = false;
  if (!decodeState(value, state)) return;
  digitalWrite(pin, state ? HIGH : LOW);
  result += label;
  result += state ? " ON " : " OFF ";
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String receivedTopic = topic;
  if (receivedTopic != MQTT_CTRL_TOPIC &&
      receivedTopic != GLED_CTRL_TOPIC &&
      receivedTopic != YLED_CTRL_TOPIC &&
      receivedTopic != RLED_CTRL_TOPIC) {
    return;
  }

  String message;
  message.reserve(length + 1);
  for (unsigned int index = 0; index < length; index++) {
    message += (char)payload[index];
  }

  JsonDocument document;
  DeserializationError error = deserializeJson(document, message);
  if (error) {
    lastCommand = "JSON ERROR";
    Serial.println("Invalid control JSON");
    updateStatusAndFooter();
    return;
  }

  String result;
  if (receivedTopic == MQTT_CTRL_TOPIC) {
    applyLedState(document["gled"], GREEN_LED_PIN, "G", result);
    applyLedState(document["yled"], YELLOW_LED_PIN, "Y", result);
    applyLedState(document["rled"], RED_LED_PIN, "R", result);
  } else if (receivedTopic == GLED_CTRL_TOPIC) {
    applyLedState(document["state"], GREEN_LED_PIN, "G", result);
  } else if (receivedTopic == YLED_CTRL_TOPIC) {
    applyLedState(document["state"], YELLOW_LED_PIN, "Y", result);
  } else if (receivedTopic == RLED_CTRL_TOPIC) {
    applyLedState(document["state"], RED_LED_PIN, "R", result);
  }

  result.trim();
  lastCommand = result.length() > 0 ? result : "NO COMMAND";
  Serial.print("MQTT control [");
  Serial.print(receivedTopic);
  Serial.print("]: ");
  Serial.println(message);
  updateStatusAndFooter();
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
    mqttConnected = mqttClient.subscribe(MQTT_CTRL_TOPIC) && mqttClient.subscribe(MQTT_CTRL_WILDCARD);
    connectionMessage = mqttConnected ? "MQTT CONNECTED" : "SUBSCRIBE ERROR";
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
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  screen.begin();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  drawConnectionScreen();
  connectWiFi();
  connectMQTT();
  drawStaticLayout();

  configTime(TAIWAN_GMT_OFFSET, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  updateClock();
  lastClockUpdate = millis();
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
  if (now - lastClockUpdate >= CLOCK_INTERVAL) {
    updateClock();
    lastClockUpdate = now;
  }
  if (now - lastAnimation >= ANIMATION_INTERVAL) {
    updateAnimation();
    lastAnimation = now;
  }
}
