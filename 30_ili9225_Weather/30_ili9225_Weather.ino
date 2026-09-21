#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "TFT_22_ILI9225.h"
#include <ArduinoJson.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* WEATHER_URL = "https://api.open-meteo.com/v1/forecast?latitude=22.6273&longitude=120.3014&daily=weather_code,temperature_2m_max,temperature_2m_min&timezone=Asia%2FTaipei&forecast_days=7";

const uint8_t TFT_SCK = 18;
const uint8_t TFT_MISO = 19;
const uint8_t TFT_MOSI = 23;
const uint8_t TFT_CS = 5;
const uint8_t TFT_RST = 26;
const uint8_t TFT_DC = 27;
const uint8_t TFT_LED = 25;

const uint16_t APP_BACKGROUND = 0x1082;
const uint16_t APP_PANEL = 0x2128;
const uint16_t APP_EDGE = 0x4208;
const uint16_t APP_HEADER = 0x18C3;
const uint16_t APP_CYAN = 0x07FF;
const uint16_t APP_YELLOW = 0xFFE0;
const uint16_t APP_ORANGE = 0xFD20;
const uint16_t APP_BLUE = 0x041F;
const uint16_t APP_WHITE = 0xFFFF;
const uint16_t APP_GREEN = 0x07E0;
const uint16_t APP_MUTED = 0xBDF7;
const uint16_t SCREEN_WIDTH = 176;
const uint16_t SCREEN_HEIGHT = 220;

const unsigned long WEATHER_INTERVAL = 60000;
const unsigned long ANIMATION_INTERVAL = 250;

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

struct WeatherDay {
  String date;
  int code;
  int high;
  int low;
};

WeatherDay forecast[7];
uint8_t forecastCount = 0;
String weatherStatus = "STARTING";
bool weatherReady = false;
unsigned long lastWeatherUpdate = 0;
unsigned long lastAnimation = 0;
uint8_t animationStep = 0;

const char* weatherName(int code) {
  if (code == 0) return "CLEAR";
  if (code <= 3) return "CLOUDY";
  if (code <= 48) return "FOG";
  if (code <= 57) return "DRIZZLE";
  if (code <= 67) return "RAIN";
  if (code <= 77) return "SNOW";
  if (code <= 82) return "SHOWERS";
  return "STORM";
}

uint16_t weatherColor(int code) {
  if (code == 0) return APP_YELLOW;
  if (code <= 48) return APP_MUTED;
  if (code <= 57) return APP_CYAN;
  if (code <= 77) return APP_CYAN;
  if (code <= 82) return APP_BLUE;
  return APP_ORANGE;
}

void drawWeatherIcon(int16_t x, int16_t y, int code) {
  uint16_t color = weatherColor(code);
  if (code == 0) {
    screen.fillCircle(x + 8, y + 8, 5, color);
    screen.drawLine(x + 8, y, x + 8, y + 16, color);
    screen.drawLine(x, y + 8, x + 16, y + 8, color);
  } else {
    screen.fillCircle(x + 7, y + 10, 5, color);
    screen.fillCircle(x + 13, y + 8, 6, color);
    screen.fillRect(x + 5, y + 10, 13, 6, color);
    if (code >= 51 && code <= 99) {
      screen.drawLine(x + 7, y + 17, x + 5, y + 21, color);
      screen.drawLine(x + 13, y + 17, x + 11, y + 21, color);
    } else {
      screen.drawLine(x + 4, y + 19, x + 18, y + 19, color);
    }
  }
}

void drawHeader() {
  screen.fillRect(0, 0, SCREEN_WIDTH, 37, APP_HEADER);
  screen.fillRect(0, 34, SCREEN_WIDTH, 3, APP_CYAN);
  screen.setTextSize(2);
  screen.setTextColor(APP_WHITE);
  screen.setCursor(20, 8);
  screen.print("KAOHSIUNG");
}

void drawStatus(const String& message, uint16_t color) {
  screen.fillRect(6, 38, 164, 9, APP_BACKGROUND);
  screen.setTextSize(1);
  screen.setTextColor(color);
  screen.setCursor(8, 39);
  screen.print(message);
}

void drawForecast() {
  screen.fillRect(0, 38, SCREEN_WIDTH, 181, APP_BACKGROUND);
  if (!weatherReady || forecastCount == 0) {
    drawStatus(weatherStatus, APP_ORANGE);
    return;
  }

  drawStatus(weatherStatus, APP_GREEN);
  screen.setTextSize(1);
  for (uint8_t index = 0; index < forecastCount; index++) {
    int16_t y = 48 + index * 23;
    screen.fillRoundRect(6, y, 164, 21, 3, APP_PANEL);
    screen.drawRoundRect(6, y, 164, 21, 3, APP_EDGE);
    drawWeatherIcon(12, y + 2, forecast[index].code);
    String dateText = forecast[index].date.substring(5);
    String tempText = String(forecast[index].high) + "/" + String(forecast[index].low) + "C";
    screen.setTextColor(APP_WHITE);
    screen.setCursor(35, y + 4);
    screen.print(dateText);
    screen.setTextColor(weatherColor(forecast[index].code));
    screen.setCursor(78, y + 4);
    screen.print(weatherName(forecast[index].code));
    screen.setTextColor(APP_WHITE);
    screen.setCursor(127, y + 4);
    screen.print(tempText);
  }
}

void updateAnimation() {
  screen.fillRect(140, 6, 31, 23, APP_HEADER);
  const int16_t positions[] = {146, 155, 164};
  for (uint8_t index = 0; index < 3; index++) {
    uint16_t color = index == animationStep ? APP_CYAN : APP_EDGE;
    uint8_t radius = index == animationStep ? 3 : 2;
    screen.fillCircle(positions[index], 17, radius, color);
  }
  animationStep = (animationStep + 1) % 3;
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    weatherStatus = "WIFI ERROR";
    weatherReady = false;
    drawForecast();
    return false;
  }

  weatherStatus = "DOWNLOADING";
  drawForecast();
  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  HTTPClient http;
  if (!http.begin(secureClient, WEATHER_URL)) {
    weatherStatus = "HTTP BEGIN ERR";
    weatherReady = false;
    drawForecast();
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    weatherStatus = "HTTP " + String(httpCode);
    weatherReady = false;
    http.end();
    drawForecast();
    return false;
  }

  String response = http.getString();
  http.end();
  JsonDocument document;
  DeserializationError error = deserializeJson(document, response);
  if (error) {
    weatherStatus = "JSON ERROR";
    weatherReady = false;
    drawForecast();
    return false;
  }

  JsonObject daily = document["daily"];
  JsonArray dates = daily["time"].as<JsonArray>();
  JsonArray codes = daily["weather_code"].as<JsonArray>();
  JsonArray highs = daily["temperature_2m_max"].as<JsonArray>();
  JsonArray lows = daily["temperature_2m_min"].as<JsonArray>();
  size_t count = dates.size();
  if (count > 7) count = 7;
  if (count == 0 || codes.size() < count || highs.size() < count || lows.size() < count) {
    weatherStatus = "DATA ERROR";
    weatherReady = false;
    drawForecast();
    return false;
  }

  forecastCount = (uint8_t)count;
  for (uint8_t index = 0; index < forecastCount; index++) {
    forecast[index].date = dates[index].as<String>();
    forecast[index].code = codes[index].as<int>();
    forecast[index].high = (int)round(highs[index].as<float>());
    forecast[index].low = (int)round(lows[index].as<float>());
  }
  weatherStatus = "UPDATED KHH";
  weatherReady = true;
  drawForecast();
  return true;
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  weatherStatus = "WIFI CONNECT";
  drawForecast();
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
  }
  weatherStatus = WiFi.status() == WL_CONNECTED ? "WIFI OK" : "WIFI ERROR";
  drawForecast();
}

void setup() {
  Serial.begin(115200);
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  screen.begin();
  drawHeader();
  drawStatus("STARTING", APP_MUTED);
  connectWiFi();
  fetchWeather();
  updateAnimation();
  lastWeatherUpdate = millis();
  lastAnimation = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastWeatherUpdate >= WEATHER_INTERVAL) {
    fetchWeather();
    lastWeatherUpdate = now;
  }
  if (now - lastAnimation >= ANIMATION_INTERVAL) {
    updateAnimation();
    lastAnimation = now;
  }
}
