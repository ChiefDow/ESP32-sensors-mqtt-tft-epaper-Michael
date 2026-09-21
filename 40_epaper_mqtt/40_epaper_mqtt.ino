#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "epd2in9b_V4.h"
#include "SimpleDHT.h"
#include "ming20_bold_assets.h"

// Waveshare 2.9-inch tri-color e-paper, landscape orientation.
// ESP32 wiring: MOSI/DIN=23, SCK=18, CS=27, DC=26, RST=25, BUSY=34.
static constexpr int DHT_PIN = 14;
static constexpr int LIGHT_PIN = 33;
static constexpr unsigned long UPDATE_INTERVAL = 60000UL;
static constexpr unsigned long MQTT_INTERVAL = 10000UL;
static constexpr size_t FRAME_BYTES = (EPD_WIDTH / 8) * EPD_HEIGHT;

// WiFi / MQTT settings carried over from the existing class project.
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_TOPIC = "mdow/class305/data";

static uint8_t blackImage[FRAME_BYTES];
static uint8_t redImage[FRAME_BYTES];
static Epd epd;
static SimpleDHT11 dht11(DHT_PIN);
static unsigned long lastUpdate = 0;
static unsigned long lastMqttPublish = 0;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static String mqttClientId;

void mapLandscapeToBuffer(int x, int y, int &bufferX, int &bufferY) {
  bufferX = y;
  bufferY = EPD_HEIGHT - 1 - x;
}

void setBlackPixel(int x, int y) {
  if (x < 0 || x >= EPD_HEIGHT || y < 0 || y >= EPD_WIDTH) return;
  int bx, by;
  mapLandscapeToBuffer(x, y, bx, by);
  const size_t index = static_cast<size_t>(by) * (EPD_WIDTH / 8) + bx / 8;
  blackImage[index] &= static_cast<uint8_t>(~(0x80 >> (bx % 8)));
}

void setRedPixel(int x, int y) {
  if (x < 0 || x >= EPD_HEIGHT || y < 0 || y >= EPD_WIDTH) return;
  int bx, by;
  mapLandscapeToBuffer(x, y, bx, by);
  const size_t index = static_cast<size_t>(by) * (EPD_WIDTH / 8) + bx / 8;
  redImage[index] &= static_cast<uint8_t>(~(0x80 >> (bx % 8)));
}

void setWhitePixel(int x, int y) {
  if (x < 0 || x >= EPD_HEIGHT || y < 0 || y >= EPD_WIDTH) return;
  int bx, by;
  mapLandscapeToBuffer(x, y, bx, by);
  const size_t index = static_cast<size_t>(by) * (EPD_WIDTH / 8) + bx / 8;
  redImage[index] |= static_cast<uint8_t>(0x80 >> (bx % 8));
}

void fillRedRect(int x0, int y0, int x1, int y1) {
  for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x)
      setRedPixel(x, y);
}

void drawRedLine(int x0, int y0, int x1, int y1) {
  const int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  const int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    setRedPixel(x0, y0);
    if (x0 == x1 && y0 == y1) break;
    const int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void drawBoldRedLine(int x0, int y0, int x1, int y1, int thickness = 3) {
  const int half = thickness / 2;
  const bool mostlyHorizontal = abs(x1 - x0) >= abs(y1 - y0);
  for (int offset = -half; offset <= half; ++offset) {
    if (mostlyHorizontal) drawRedLine(x0, y0 + offset, x1, y1 + offset);
    else drawRedLine(x0 + offset, y0, x1 + offset, y1);
  }
}

void drawRedCircle(int cx, int cy, int radius) {
  int x = radius, y = 0, err = 0;
  while (x >= y) {
    setRedPixel(cx + x, cy + y); setRedPixel(cx + y, cy + x);
    setRedPixel(cx - y, cy + x); setRedPixel(cx - x, cy + y);
    setRedPixel(cx - x, cy - y); setRedPixel(cx - y, cy - x);
    setRedPixel(cx + y, cy - x); setRedPixel(cx + x, cy - y);
    ++y;
    if (err <= 0) err += 2 * y + 1;
    if (err > 0) { --x; err -= 2 * x + 1; }
  }
}

void fillRedCircle(int cx, int cy, int radius) {
  for (int y = -radius; y <= radius; ++y) {
    const int halfWidth = static_cast<int>(sqrt(radius * radius - y * y));
    for (int x = -halfWidth; x <= halfWidth; ++x) setRedPixel(cx + x, cy + y);
  }
}

void drawThermometerIcon(int cx, int top) {
  // Compact version for the second matrix row.
  drawRedCircle(cx, top + 5, 6);
  drawRedCircle(cx, top + 5, 5);
  drawBoldRedLine(cx - 5, top + 5, cx - 5, top + 21, 2);
  drawBoldRedLine(cx + 5, top + 5, cx + 5, top + 21, 2);
  fillRedCircle(cx, top + 22, 8);
  for (int y = top + 5; y <= top + 18; ++y) {
    for (int x = cx - 2; x <= cx + 2; ++x) setWhitePixel(x, y);
  }
}

void drawHumidityIcon(int cx, int top) {
  // Upright solid water drop: pointed top and rounded lower bowl.
  const int baseY = top + 1;
  for (int row = 0; row <= 28; ++row) {
    int halfWidth;
    if (row <= 12) {
      halfWidth = (row * 10) / 12;
    } else {
      const int dy = row - 12;
      halfWidth = static_cast<int>(sqrt(10 * 10 - min(dy, 10) * min(dy, 10)));
    }
    for (int x = -halfWidth; x <= halfWidth; ++x) {
      setRedPixel(cx + x, baseY + row);
    }
  }
}

void drawBrightnessIcon(int cx, int top) {
  // Reference-style brightness icon: ring with four rounded directional nodes.
  const int cy = top + 14;
  drawBoldRedLine(cx - 9, cy, cx + 9, cy, 2);
  drawBoldRedLine(cx, cy - 9, cx, cy + 9, 2);
  fillRedCircle(cx - 14, cy, 4);
  fillRedCircle(cx + 14, cy, 4);
  fillRedCircle(cx, cy - 14, 4);
  fillRedCircle(cx, cy + 14, 4);
  drawRedCircle(cx, cy, 10);
  drawRedCircle(cx, cy, 9);
}

void drawKai20(int glyphIndex, int x, int y) {
  const uint8_t *glyph = MING20_BOLD_GLYPHS[glyphIndex];
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 20; ++col) {
      if (glyph[row * 3 + col / 8] & (0x80 >> (col % 8))) {
        setBlackPixel(x + col, y + row);
      }
    }
  }
}

void drawKai20White(int glyphIndex, int x, int y) {
  const uint8_t *glyph = MING20_BOLD_GLYPHS[glyphIndex];
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 20; ++col) {
      if (glyph[row * 3 + col / 8] & (0x80 >> (col % 8))) {
        setWhitePixel(x + col, y + row);
      }
    }
  }
}

// ASCII glyphs are horizontally fitted to 14 pixels while retaining a 20px
// bold MingLiU height, so readings remain inside each 98px column.
void drawKai20Ascii(int glyphIndex, int x, int y) {
  const uint8_t *glyph = MING20_BOLD_GLYPHS[glyphIndex];
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 14; ++col) {
      const int sourceCol = (col * 20) / 14;
      if (glyph[row * 3 + sourceCol / 8] & (0x80 >> (sourceCol % 8))) {
        setBlackPixel(x + col, y + row);
      }
    }
  }
}

void drawKai20AsciiWhite(int glyphIndex, int x, int y) {
  const uint8_t *glyph = MING20_BOLD_GLYPHS[glyphIndex];
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 14; ++col) {
      const int sourceCol = (col * 20) / 14;
      if (glyph[row * 3 + sourceCol / 8] & (0x80 >> (sourceCol % 8))) {
        setWhitePixel(x + col, y + row);
      }
    }
  }
}

void drawTitleWhite() {
  // ESP32 環境監測, centered in the red first matrix row.
  const int startX = 69;
  const int ascii[] = {13, 14, 15, 3, 2}; // E S P 3 2
  for (int i = 0; i < 5; ++i) drawKai20AsciiWhite(ascii[i], startX + i * 14, 5);
  const int cjk[] = {17, 18, 19, 20}; // 環 境 監 測
  for (int i = 0; i < 4; ++i) drawKai20White(cjk[i], startX + 78 + i * 20, 5);
}

void drawLabelWhite(int centerX, int firstGlyph, int secondGlyph) {
  drawKai20White(firstGlyph, centerX - 20, 70);
  drawKai20White(secondGlyph, centerX, 70);
}

void drawReading(int centerX, bool valid, int value, bool temperature) {
  if (!valid) {
    for (int i = 0; i < 3; ++i) drawKai20Ascii(10, centerX - 21 + i * 14, 102);
    return;
  }
  char digits[8];
  snprintf(digits, sizeof(digits), "%d", value);
  const int count = static_cast<int>(strlen(digits));
  int width = count * 14 + 14;
  if (temperature) width += 28; // degree + C
  int x = centerX - width / 2;
  for (int i = 0; i < count; ++i) drawKai20Ascii(digits[i] - '0', x + i * 14, 102);
  x += count * 14;
  if (temperature) {
    drawKai20Ascii(16, x, 102); // °
    drawKai20Ascii(12, x + 14, 102); // C
  } else {
    drawKai20Ascii(11, x, 102); // %
  }
}

void drawScreen(bool tempValid, int temperature, bool humidityValid, int humidity,
                bool lightValid, int brightness) {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  const int centers[3] = {49, 148, 247};
  // 4x3 matrix: rows are 0-31, 32-63, 64-95 and 96-127.
  fillRedRect(0, 0, EPD_HEIGHT - 1, 31);     // a11-a13
  fillRedRect(0, 64, EPD_HEIGHT - 1, 95);    // a31-a33
  drawTitleWhite();

  drawThermometerIcon(centers[0], 34);       // a21
  drawHumidityIcon(centers[1], 34);          // a22
  drawBrightnessIcon(centers[2], 34);        // a23

  drawLabelWhite(centers[0], 21, 22);         // a31: 溫度
  drawLabelWhite(centers[1], 23, 22);         // a32: 濕度
  drawLabelWhite(centers[2], 24, 22);         // a33: 亮度

  drawReading(centers[0], tempValid, temperature, true);  // a41: °C
  drawReading(centers[1], humidityValid, humidity, false); // a42: %
  drawReading(centers[2], lightValid, brightness, false);   // a43: %

  // Matrix separators: two column lines and three row lines.
  for (int y = 32; y < EPD_WIDTH; ++y) {
    setBlackPixel(98, y);
    setBlackPixel(197, y);
  }
  for (int x = 0; x < EPD_HEIGHT; ++x) {
    setBlackPixel(x, 32);
    setBlackPixel(x, 64);
    setBlackPixel(x, 96);
  }
}

void updateDisplay() {
  byte temperature = 0;
  byte humidity = 0;
  const int dhtError = dht11.read(&temperature, &humidity, NULL);
  const bool tempValid = (dhtError == SimpleDHTErrSuccess);
  const bool humidityValid = tempValid;

  const int rawLight = analogRead(LIGHT_PIN);
  const bool lightValid = (rawLight >= 0 && rawLight <= 4095);
  // Calibrated so a stronger light level produces a larger percentage.
  const int brightness = lightValid ? constrain(map(rawLight, 0, 4095, 100, 0), 0, 100) : 0;

  Serial.printf("DHT: %s, temperature=%d, humidity=%d; light=%s, brightness=%d%%\n",
                tempValid ? "OK" : "--", temperature, humidity,
                lightValid ? "OK" : "--", brightness);

  drawScreen(tempValid, temperature, humidityValid, humidity, lightValid, brightness);
  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }
  epd.Display(blackImage, redImage);
  epd.Sleep();
}

int readLightPercent(bool &valid) {
  const int rawLight = analogRead(LIGHT_PIN);
  valid = (rawLight >= 0 && rawLight <= 4095);
  // Calibrated so a stronger light level produces a larger percentage.
  return valid ? constrain(map(rawLight, 0, 4095, 100, 0), 0, 100) : 0;
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000UL) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection timeout");
  }
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;
  mqttClientId = String("esp32-epaper-") + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF), HEX);
  Serial.print("Connecting MQTT: ");
  Serial.println(MQTT_HOST);
  if (mqttClient.connect(mqttClientId.c_str())) {
    Serial.println("MQTT connected");
  } else {
    Serial.print("MQTT connection failed, state=");
    Serial.println(mqttClient.state());
  }
}

void publishSensorData() {
  if (!mqttClient.connected()) return;

  byte temperature = 0;
  byte humidity = 0;
  const int dhtError = dht11.read(&temperature, &humidity, NULL);
  const bool dhtValid = (dhtError == SimpleDHTErrSuccess);
  bool lightValid = false;
  const int lightPercent = readLightPercent(lightValid);

  String payload = "{\"temp\":";
  payload += dhtValid ? String(temperature) : "null";
  payload += ",\"humi\":";
  payload += dhtValid ? String(humidity) : "null";
  payload += ",\"light\":";
  payload += lightValid ? String(lightPercent) : "null";
  payload += "}";

  const bool published = mqttClient.publish(MQTT_TOPIC, payload.c_str());
  Serial.print("MQTT ");
  Serial.print(published ? "published: " : "publish failed: ");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(34, INPUT); // GPIO34 is input-only; BUSY must not use INPUT_PULLUP.
  analogReadResolution(12);
  delay(100);
  Serial.println("40_epaper_mqtt: temperature / humidity / brightness");
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectWiFi();
  connectMQTT();
  updateDisplay();
  lastUpdate = millis();
  lastMqttPublish = millis() - MQTT_INTERVAL;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  if (millis() - lastMqttPublish >= MQTT_INTERVAL) {
    publishSensorData();
    lastMqttPublish = millis();
  }
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    updateDisplay();
    lastUpdate = millis();
  }
  delay(100);
}
