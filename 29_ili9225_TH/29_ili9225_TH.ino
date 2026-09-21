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

const uint16_t COLOR_BACKGROUND = 0x1082;
const uint16_t COLOR_PANEL = 0x2128;
const uint16_t COLOR_PANEL_EDGE = 0x4208;
const uint16_t COLOR_HEADER = 0x18C3;
const uint16_t COLOR_RED_ACCENT = 0xF800;
const uint16_t APP_COLOR_ORANGE = 0xFD20;
const uint16_t APP_COLOR_CYAN = 0x07FF;
const uint16_t COLOR_GREEN_ACCENT = 0x07E0;
const uint16_t COLOR_MUTED = 0xBDF7;
const uint16_t SCREEN_WIDTH = 176;
const uint16_t SCREEN_HEIGHT = 220;

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
int dhtError = SimpleDHTErrSuccess;
unsigned long lastRead = 0;
unsigned long lastAnimation = 0;
uint8_t animationStep = 0;

void drawThermometerIcon(int16_t x, int16_t y, uint16_t color) {
  screen.drawRoundRect(x + 8, y, 9, 31, 4, color);
  screen.fillCircle(x + 12, y + 35, 9, color);
  screen.fillRect(x + 10, y + 12, 5, 25, color);
  screen.drawLine(x + 20, y + 6, x + 25, y + 6, color);
  screen.drawLine(x + 20, y + 15, x + 25, y + 15, color);
  screen.drawLine(x + 20, y + 24, x + 25, y + 24, color);
}

void drawDropIcon(int16_t x, int16_t y, uint16_t color) {
  screen.fillTriangle(x + 18, y, x + 2, y + 26, x + 34, y + 26, color);
  screen.fillCircle(x + 18, y + 26, 16, color);
  screen.fillCircle(x + 18, y + 28, 8, COLOR_PANEL);
}

void drawStaticScreen() {
  screen.fillScreen(COLOR_BACKGROUND);

  screen.fillRect(0, 0, SCREEN_WIDTH, 36, COLOR_HEADER);
  screen.fillRect(0, 34, SCREEN_WIDTH, 3, APP_COLOR_CYAN);
  screen.setTextSize(2);
  screen.setTextColor(COLOR_WHITE);
  screen.setCursor(22, 9);
  screen.print("WEATHER");

  screen.fillRoundRect(7, 44, 162, 73, 6, COLOR_PANEL);
  screen.drawRoundRect(7, 44, 162, 73, 6, COLOR_RED_ACCENT);
  drawThermometerIcon(17, 59, COLOR_RED_ACCENT);
  screen.setTextSize(1);
  screen.setTextColor(COLOR_MUTED);
  screen.setCursor(57, 54);
  screen.print("TEMPERATURE");

  screen.fillRoundRect(7, 127, 162, 73, 6, COLOR_PANEL);
  screen.drawRoundRect(7, 127, 162, 73, 6, APP_COLOR_CYAN);
  drawDropIcon(17, 141, APP_COLOR_CYAN);
  screen.setTextColor(COLOR_MUTED);
  screen.setCursor(57, 137);
  screen.print("HUMIDITY");

  screen.setTextSize(1);
  screen.setTextColor(COLOR_MUTED);
  screen.setCursor(8, 205);
  screen.print("LIVE SENSOR");
}

void drawSensorValues() {
  screen.fillRect(55, 70, 103, 31, COLOR_PANEL);
  screen.fillRect(55, 153, 103, 31, COLOR_PANEL);
  screen.setTextSize(3);

  if (dhtError == SimpleDHTErrSuccess) {
    screen.setTextColor(COLOR_WHITE);
    screen.setCursor(57, 73);
    screen.print(temperature);
    screen.print(" C");
    screen.setCursor(57, 156);
    screen.print(humidity);
    screen.print(" %");

    screen.fillRect(57, 185, 101, 10, COLOR_PANEL);
    screen.drawRoundRect(57, 185, 101, 10, 3, COLOR_PANEL_EDGE);
    uint16_t fillWidth = ((uint32_t)95 * humidity) / 100;
    if (fillWidth > 0) {
      screen.fillRoundRect(60, 188, fillWidth, 4, 2, APP_COLOR_CYAN);
    }
  } else {
    screen.setTextColor(APP_COLOR_ORANGE);
    screen.setCursor(57, 77);
    screen.print("ERROR");
    screen.setCursor(57, 160);
    screen.print("ERROR");
    screen.fillRect(57, 185, 101, 10, COLOR_PANEL);
    screen.drawRoundRect(57, 185, 101, 10, 3, COLOR_PANEL_EDGE);
  }
}

void updateStatusAnimation() {
  screen.fillRect(137, 202, 32, 17, COLOR_BACKGROUND);
  const int16_t positions[] = {143, 153, 163};
  for (uint8_t index = 0; index < 3; index++) {
    uint16_t color = index == animationStep ? APP_COLOR_CYAN : COLOR_PANEL_EDGE;
    uint8_t radius = index == animationStep ? 4 : 2;
    screen.fillCircle(positions[index], 211, radius, color);
  }
  animationStep = (animationStep + 1) % 3;
}

void readSensor() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  drawSensorValues();
  screen.fillRect(8, 202, 120, 17, COLOR_BACKGROUND);
  screen.setTextSize(1);
  screen.setCursor(8, 205);
  if (dhtError == SimpleDHTErrSuccess) {
    screen.setTextColor(COLOR_GREEN_ACCENT);
    screen.print("LIVE SENSOR");
  } else {
    screen.setTextColor(APP_COLOR_ORANGE);
    screen.print("CHECK DHT11");
  }
}

void setup() {
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  screen.begin();
  drawStaticScreen();
  readSensor();
  updateStatusAnimation();
  lastRead = millis();
  lastAnimation = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastRead >= 2000) {
    readSensor();
    lastRead = now;
  }
  if (now - lastAnimation >= 180) {
    updateStatusAnimation();
    lastAnimation = now;
  }
}