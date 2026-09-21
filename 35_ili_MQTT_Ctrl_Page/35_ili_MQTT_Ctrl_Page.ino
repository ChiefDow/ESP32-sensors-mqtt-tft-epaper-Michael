#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <math.h>
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
const uint8_t PAGE_BUTTON_PIN = 0;

const uint16_t APP_BACKGROUND = 0x1082;
const uint16_t APP_PANEL = 0x2128;
const uint16_t APP_EDGE = 0x4208;
const uint16_t APP_HEADER = 0x18C3;
const uint16_t APP_RED = 0xF800;
const uint16_t APP_BLUE = 0x001F;
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
const uint16_t TEMP_HISTORY_SIZE = 300;
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
byte temperatureHistory[TEMP_HISTORY_SIZE];
uint16_t temperatureHistoryCount = 0;
uint16_t temperatureHistoryIndex = 0;
byte humidityHistory[TEMP_HISTORY_SIZE];
byte brightnessHistory[TEMP_HISTORY_SIZE];
uint16_t humidityHistoryCount = 0;
uint16_t humidityHistoryIndex = 0;
uint16_t brightnessHistoryCount = 0;
uint16_t brightnessHistoryIndex = 0;
uint8_t currentPage = 0;
bool lastPageButtonState = HIGH;
unsigned long lastPageButtonChange = 0;
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

void recordHistoryValue(byte* history, uint16_t& count, uint16_t& index, byte value) {
  history[index] = value;
  index = (index + 1) % TEMP_HISTORY_SIZE;
  if (count < TEMP_HISTORY_SIZE) count++;
}

void recordTemperature() {
  if (dhtError != SimpleDHTErrSuccess) return;
  temperatureHistory[temperatureHistoryIndex] = temperature;
  temperatureHistoryIndex = (temperatureHistoryIndex + 1) % TEMP_HISTORY_SIZE;
  if (temperatureHistoryCount < TEMP_HISTORY_SIZE) temperatureHistoryCount++;
}

int16_t temperatureGraphY(byte value) {
  int limitedValue = constrain((int)value, 10, 40);
  return 190 - ((limitedValue - 10) * 66) / 30;
}

void drawGaugeArc(int16_t centerX, int16_t centerY, int16_t innerRadius, int16_t outerRadius, int16_t startAngle, int16_t endAngle, uint16_t color) {
  const float degreesToRadians = 0.0174532925f;
  for (int16_t radius = innerRadius; radius <= outerRadius; radius += 2) {
    for (int16_t angle = startAngle; angle > endAngle; angle -= 2) {
      int16_t x1 = centerX + (int16_t)(cos(angle * degreesToRadians) * radius);
      int16_t y1 = centerY - (int16_t)(sin(angle * degreesToRadians) * radius);
      int16_t x2 = centerX + (int16_t)(cos((angle - 2) * degreesToRadians) * radius);
      int16_t y2 = centerY - (int16_t)(sin((angle - 2) * degreesToRadians) * radius);
      screen.drawLine(x1, y1, x2, y2, color);
    }
  }
}

void drawTemperatureGauge() {
  const int16_t centerX = 88;
  const int16_t centerY = 105;
  const int16_t innerRadius = 43;
  const int16_t outerRadius = 49;
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius, 180, 120, APP_GREEN);
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius, 120, 60, APP_YELLOW);
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius, 60, 0, APP_RED);

  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(30, 102);
  screen.print("10");
  screen.setCursor(84, 48);
  screen.print("25");
  screen.setCursor(145, 102);
  screen.print("40");

  int limitedTemperature = constrain((int)temperature, 10, 40);
  int16_t needleAngle = 180 - ((limitedTemperature - 10) * 180) / 30;
  const float degreesToRadians = 0.0174532925f;
  int16_t needleX = centerX + (int16_t)(cos(needleAngle * degreesToRadians) * 38);
  int16_t needleY = centerY - (int16_t)(sin(needleAngle * degreesToRadians) * 38);
  screen.drawLine(centerX, centerY, needleX, needleY, APP_WHITE);
  screen.fillCircle(centerX, centerY, 4, APP_WHITE);
  screen.setTextColor(APP_WHITE);
  screen.setTextSize(2);
  screen.setCursor(73, 78);
  if (dhtError == SimpleDHTErrSuccess) {
    screen.print(temperature);
    screen.print("C");
  } else {
    screen.print("--");
  }
}

void drawTemperaturePage() {
  screen.fillScreen(APP_BACKGROUND);
  drawStatusBar();
  screen.fillRect(0, 17, SCREEN_WIDTH, 30, APP_HEADER);
  screen.fillRect(0, 44, SCREEN_WIDTH, 3, APP_RED);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(22, 24);
  screen.print("TEMP 10 MIN");
  drawTemperatureGauge();

  const int16_t left = 29;
  const int16_t right = 168;
  const int16_t top = 124;
  const int16_t bottom = 190;
  screen.drawLine(left, top, left, bottom, APP_MUTED);
  screen.drawLine(left, bottom, right, bottom, APP_MUTED);
  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(5, 121);
  screen.print("40");
  screen.setCursor(5, 153);
  screen.print("25");
  screen.setCursor(5, 183);
  screen.print("10");
  screen.setCursor(29, 193);
  screen.print("10m");
  screen.setCursor(92, 193);
  screen.print("5m");
  screen.setCursor(145, 193);
  screen.print("NOW");

  screen.setTextColor(APP_RED);
  if (temperatureHistoryCount == 0) {
    screen.setCursor(52, 153);
    screen.print("WAITING");
    return;
  }

  uint16_t startIndex = temperatureHistoryCount == TEMP_HISTORY_SIZE ? temperatureHistoryIndex : 0;
  if (temperatureHistoryCount == 1) {
    uint16_t valueIndex = (startIndex + TEMP_HISTORY_SIZE - temperatureHistoryCount) % TEMP_HISTORY_SIZE;
    screen.fillCircle(left, temperatureGraphY(temperatureHistory[valueIndex]), 2, APP_RED);
    return;
  }

  for (uint16_t point = 1; point < temperatureHistoryCount; point++) {
    uint16_t previousIndex = (startIndex + point - 1) % TEMP_HISTORY_SIZE;
    uint16_t currentIndex = (startIndex + point) % TEMP_HISTORY_SIZE;
    int16_t x1 = left + ((right - left) * (point - 1)) / (temperatureHistoryCount - 1);
    int16_t x2 = left + ((right - left) * point) / (temperatureHistoryCount - 1);
    screen.drawLine(x1, temperatureGraphY(temperatureHistory[previousIndex]), x2, temperatureGraphY(temperatureHistory[currentIndex]), APP_RED);
  }
}

void updateTemperatureDataLocal() {
  screen.fillRect(25, 47, 126, 65, APP_BACKGROUND);
  drawTemperatureGauge();

  const int16_t left = 29;
  const int16_t right = 168;
  const int16_t top = 124;
  const int16_t bottom = 190;
  screen.fillRect(0, 119, SCREEN_WIDTH, 78, APP_BACKGROUND);
  screen.drawLine(left, top, left, bottom, APP_MUTED);
  screen.drawLine(left, bottom, right, bottom, APP_MUTED);
  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(5, 121);
  screen.print("40");
  screen.setCursor(5, 153);
  screen.print("25");
  screen.setCursor(5, 183);
  screen.print("10");
  screen.setCursor(29, 193);
  screen.print("10m");
  screen.setCursor(92, 193);
  screen.print("5m");
  screen.setCursor(145, 193);
  screen.print("NOW");

  screen.setTextColor(APP_RED);
  if (temperatureHistoryCount == 0) {
    screen.setCursor(52, 153);
    screen.print("WAITING");
    return;
  }

  uint16_t startIndex = temperatureHistoryCount == TEMP_HISTORY_SIZE ? temperatureHistoryIndex : 0;
  if (temperatureHistoryCount == 1) {
    uint16_t valueIndex = (startIndex + TEMP_HISTORY_SIZE - temperatureHistoryCount) % TEMP_HISTORY_SIZE;
    screen.fillCircle(left, temperatureGraphY(temperatureHistory[valueIndex]), 2, APP_RED);
    return;
  }

  for (uint16_t point = 1; point < temperatureHistoryCount; point++) {
    uint16_t previousIndex = (startIndex + point - 1) % TEMP_HISTORY_SIZE;
    uint16_t currentIndex = (startIndex + point) % TEMP_HISTORY_SIZE;
    int16_t x1 = left + ((right - left) * (point - 1)) / (temperatureHistoryCount - 1);
    int16_t x2 = left + ((right - left) * point) / (temperatureHistoryCount - 1);
    screen.drawLine(x1, temperatureGraphY(temperatureHistory[previousIndex]), x2, temperatureGraphY(temperatureHistory[currentIndex]), APP_RED);
  }
}
int16_t percentGraphY(byte value) {
  int limitedValue = constrain((int)value, 0, 100);
  return 190 - (limitedValue * 66) / 100;
}

void drawPercentGauge(int value, uint16_t lowColor, uint16_t middleColor, uint16_t highColor, int firstLimit, int secondLimit) {
  const int16_t centerX = 88;
  const int16_t centerY = 105;
  int firstAngle = 180 - (firstLimit * 180) / 100;
  int secondAngle = 180 - (secondLimit * 180) / 100;
  drawGaugeArc(centerX, centerY, 43, 49, 180, firstAngle, lowColor);
  drawGaugeArc(centerX, centerY, 43, 49, firstAngle, secondAngle, middleColor);
  drawGaugeArc(centerX, centerY, 43, 49, secondAngle, 0, highColor);

  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(27, 102);
  screen.print("0");
  screen.setCursor(84, 48);
  screen.print("50");
  screen.setCursor(145, 102);
  screen.print("100");

  int limitedValue = constrain(value, 0, 100);
  int16_t needleAngle = 180 - (limitedValue * 180) / 100;
  const float degreesToRadians = 0.0174532925f;
  int16_t needleX = centerX + (int16_t)(cos(needleAngle * degreesToRadians) * 38);
  int16_t needleY = centerY - (int16_t)(sin(needleAngle * degreesToRadians) * 38);
  screen.drawLine(centerX, centerY, needleX, needleY, APP_WHITE);
  screen.fillCircle(centerX, centerY, 4, APP_WHITE);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  String valueText = String(limitedValue) + "%";
  int16_t textX = 88 - (valueText.length() * 6);
  screen.setCursor(textX, 78);
  screen.print(valueText);
}

void drawHistoryGraph(const byte* history, uint16_t count, uint16_t index, uint16_t lineColor) {
  const int16_t left = 29;
  const int16_t right = 168;
  const int16_t top = 124;
  const int16_t bottom = 190;
  screen.drawLine(left, top, left, bottom, APP_MUTED);
  screen.drawLine(left, bottom, right, bottom, APP_MUTED);
  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(5, 121);
  screen.print("100");
  screen.setCursor(5, 153);
  screen.print("50");
  screen.setCursor(5, 183);
  screen.print("0");
  screen.setCursor(29, 193);
  screen.print("10m");
  screen.setCursor(92, 193);
  screen.print("5m");
  screen.setCursor(145, 193);
  screen.print("NOW");
  if (count == 0) {
    screen.setTextColor(lineColor);
    screen.setCursor(52, 153);
    screen.print("WAITING");
    return;
  }
  uint16_t startIndex = count == TEMP_HISTORY_SIZE ? index : 0;
  if (count == 1) {
    uint16_t valueIndex = (startIndex + TEMP_HISTORY_SIZE - count) % TEMP_HISTORY_SIZE;
    screen.fillCircle(left, percentGraphY(history[valueIndex]), 2, lineColor);
    return;
  }
  for (uint16_t point = 1; point < count; point++) {
    uint16_t previousIndex = (startIndex + point - 1) % TEMP_HISTORY_SIZE;
    uint16_t currentIndex = (startIndex + point) % TEMP_HISTORY_SIZE;
    int16_t x1 = left + ((right - left) * (point - 1)) / (count - 1);
    int16_t x2 = left + ((right - left) * point) / (count - 1);
    screen.drawLine(x1, percentGraphY(history[previousIndex]), x2, percentGraphY(history[currentIndex]), lineColor);
  }
}

void drawPercentPage(const char* title, int value, uint16_t lowColor, uint16_t middleColor, uint16_t highColor, int firstLimit, int secondLimit, const byte* history, uint16_t count, uint16_t index, uint16_t lineColor) {
  screen.fillScreen(APP_BACKGROUND);
  drawStatusBar();
  screen.fillRect(0, 17, SCREEN_WIDTH, 30, APP_HEADER);
  screen.fillRect(0, 44, SCREEN_WIDTH, 3, lineColor);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(14, 24);
  screen.print(title);
  drawPercentGauge(value, lowColor, middleColor, highColor, firstLimit, secondLimit);
  screen.fillRect(0, 119, SCREEN_WIDTH, 78, APP_BACKGROUND);
  drawHistoryGraph(history, count, index, lineColor);
}

void updatePercentPageData(int value, uint16_t lowColor, uint16_t middleColor, uint16_t highColor, int firstLimit, int secondLimit, const byte* history, uint16_t count, uint16_t index, uint16_t lineColor) {
  screen.fillRect(25, 47, 126, 65, APP_BACKGROUND);
  drawPercentGauge(value, lowColor, middleColor, highColor, firstLimit, secondLimit);
  screen.fillRect(0, 119, SCREEN_WIDTH, 78, APP_BACKGROUND);
  drawHistoryGraph(history, count, index, lineColor);
}

void drawCurrentPage() {
  if (currentPage == 0) {
    drawStaticLayout();
    drawSensorValues();
  } else if (currentPage == 1) {
    drawTemperaturePage();
  } else if (currentPage == 2) {
    drawPercentPage(
      "HUMIDITY 10M",
      humidity,
      APP_YELLOW,
      APP_GREEN,
      APP_RED,
      50,
      60,
      humidityHistory,
      humidityHistoryCount,
      humidityHistoryIndex,
      APP_CYAN
    );
  } else {
    drawPercentPage(
      "LIGHT 10 MIN",
      brightness,
      APP_BLUE,
      APP_GREEN,
      APP_YELLOW,
      40,
      60,
      brightnessHistory,
      brightnessHistoryCount,
      brightnessHistoryIndex,
      APP_YELLOW
    );
  }
  updateStatusAndFooter();
}

void handlePageButton() {
  bool buttonState = digitalRead(PAGE_BUTTON_PIN);
  unsigned long now = millis();
  if (buttonState != lastPageButtonState && now - lastPageButtonChange > 50) {
    lastPageButtonChange = now;
    lastPageButtonState = buttonState;
    if (buttonState == LOW) {
      currentPage = (currentPage + 1) % 4;
      drawCurrentPage();
    }
  }
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
  recordTemperature();
  if (dhtError == SimpleDHTErrSuccess) {
    recordHistoryValue(humidityHistory, humidityHistoryCount, humidityHistoryIndex, humidity);
  }
  recordHistoryValue(brightnessHistory, brightnessHistoryCount, brightnessHistoryIndex, (byte)brightness);
  if (currentPage == 0) {
    drawSensorValues();
  } else if (currentPage == 1) {
    updateTemperatureDataLocal();
  } else if (currentPage == 2) {
    updatePercentPageData(
      humidity,
      APP_YELLOW,
      APP_GREEN,
      APP_RED,
      50,
      60,
      humidityHistory,
      humidityHistoryCount,
      humidityHistoryIndex,
      APP_CYAN
    );
  } else {
    updatePercentPageData(
      brightness,
      APP_BLUE,
      APP_GREEN,
      APP_YELLOW,
      40,
      60,
      brightnessHistory,
      brightnessHistoryCount,
      brightnessHistoryIndex,
      APP_YELLOW
    );
  }
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
  pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);
  lastPageButtonState = digitalRead(PAGE_BUTTON_PIN);
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
  drawCurrentPage();

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
  handlePageButton();
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    connectMQTT();
    drawCurrentPage();
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
