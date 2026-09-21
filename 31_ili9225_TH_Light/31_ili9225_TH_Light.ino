#include <SPI.h>
#include "Adafruit_GFX.h"
#include "TFT_22_ILI9225.h"
#include "SimpleDHT.h"

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
const unsigned long ANIMATION_INTERVAL = 220;

TFT_22_ILI9225 ili9225(TFT_RST, TFT_DC, TFT_CS, TFT_LED);

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
SimpleDHT11 dht11(DHT_PIN);

byte temperature = 0;
byte humidity = 0;
int brightness = 0;
int dhtError = SimpleDHTErrSuccess;
unsigned long lastSensorRead = 0;
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

void drawStaticLayout() {
  screen.fillScreen(APP_BACKGROUND);
  screen.fillRect(0, 0, SCREEN_WIDTH, 38, APP_HEADER);
  screen.fillRect(0, 35, SCREEN_WIDTH, 3, APP_CYAN);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(17, 9);
  screen.print("ENV MONITOR");

  screen.fillRoundRect(7, 46, 162, 54, 6, APP_PANEL);
  screen.drawRoundRect(7, 46, 162, 54, 6, APP_RED);
  drawThermometerIcon(18, 57, APP_RED);
  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(51, 55);
  screen.print("TEMPERATURE");

  screen.fillRoundRect(7, 108, 80, 88, 6, APP_PANEL);
  screen.drawRoundRect(7, 108, 80, 88, 6, APP_CYAN);
  drawDropIcon(17, 119, APP_CYAN);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(17, 148);
  screen.print("HUMIDITY");

  screen.fillRoundRect(89, 108, 80, 88, 6, APP_PANEL);
  screen.drawRoundRect(89, 108, 80, 88, 6, APP_YELLOW);
  drawSunIcon(108, 118, APP_YELLOW);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(101, 148);
  screen.print("LIGHT");

  screen.setTextSize(1);
  screen.setTextColor(APP_MUTED);
  screen.setCursor(8, 204);
  screen.print("DHT11  /  LDR SENSOR");
}

void drawTemperatureValue() {
  screen.fillRect(50, 70, 108, 25, APP_PANEL);
  screen.setTextSize(3);
  if (dhtError == SimpleDHTErrSuccess) {
    screen.setTextColor(APP_WHITE);
    screen.setCursor(53, 70);
    screen.print(temperature);
    screen.print(" C");
  } else {
    screen.setTextColor(APP_ORANGE);
    screen.setCursor(53, 73);
    screen.print("ERROR");
  }
}

void drawHumidityValue() {
  screen.fillRect(13, 162, 68, 28, APP_PANEL);
  screen.setTextSize(2);
  if (dhtError == SimpleDHTErrSuccess) {
    screen.setTextColor(APP_WHITE);
    screen.setCursor(19, 164);
    screen.print(humidity);
    screen.print("%");
  } else {
    screen.setTextColor(APP_ORANGE);
    screen.setCursor(18, 168);
    screen.print("ERR");
  }
}

void drawBrightnessValue() {
  screen.fillRect(95, 162, 68, 28, APP_PANEL);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(103, 164);
  screen.print(brightness);
  screen.print("%");
  screen.fillRect(98, 184, 62, 7, APP_PANEL);
  screen.drawRoundRect(98, 184, 62, 7, 2, APP_EDGE);
  uint16_t barWidth = ((uint32_t)56 * brightness) / 100;
  if (barWidth > 0) screen.fillRoundRect(101, 186, barWidth, 3, 1, APP_YELLOW);
}

void drawSensorValues() {
  drawTemperatureValue();
  drawHumidityValue();
  drawBrightnessValue();
  screen.fillRect(8, 204, 132, 12, APP_BACKGROUND);
  screen.setTextSize(1);
  screen.setCursor(8, 205);
  if (dhtError == SimpleDHTErrSuccess) {
    screen.setTextColor(APP_GREEN);
    screen.print("SENSOR ONLINE");
  } else {
    screen.setTextColor(APP_ORANGE);
    screen.print("DHT CHECK");
  }
}

void updateAnimation() {
  screen.fillRect(143, 202, 27, 17, APP_BACKGROUND);
  const int16_t positions[] = {148, 157, 166};
  for (uint8_t index = 0; index < 3; index++) {
    uint16_t color = index == animationStep ? APP_CYAN : APP_EDGE;
    uint8_t radius = index == animationStep ? 3 : 2;
    screen.fillCircle(positions[index], 210, radius, color);
  }
  animationStep = (animationStep + 1) % 3;
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  int lightRaw = analogRead(LIGHT_PIN);
  brightness = map(lightRaw, 4095, 0, 0, 100);
  brightness = constrain(brightness, 0, 100);
  drawSensorValues();
}

void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  screen.begin();
  drawStaticLayout();
  readSensors();
  updateAnimation();
  lastSensorRead = millis();
  lastAnimation = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    lastSensorRead = now;
  }
  if (now - lastAnimation >= ANIMATION_INTERVAL) {
    updateAnimation();
    lastAnimation = now;
  }
}