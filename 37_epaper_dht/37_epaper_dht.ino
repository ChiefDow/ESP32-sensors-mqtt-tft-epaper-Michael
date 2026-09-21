#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include "epd2in9b_V4.h"
#include "SimpleDHT.h"
#include "ming20_bold_assets.h"

// Waveshare 2.9-inch tri-color e-paper, landscape orientation.
// ESP32 wiring: MOSI/DIN=23, SCK=18, CS=27, DC=26, RST=25, BUSY=34.
static constexpr int DHT_PIN = 14;
static constexpr int LIGHT_PIN = 33;
static constexpr unsigned long UPDATE_INTERVAL = 60000UL;
static constexpr size_t FRAME_BYTES = (EPD_WIDTH / 8) * EPD_HEIGHT;

static uint8_t blackImage[FRAME_BYTES];
static uint8_t redImage[FRAME_BYTES];
static Epd epd;
static SimpleDHT11 dht11(DHT_PIN);
static unsigned long lastUpdate = 0;

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
  // Reference-style thermometer: strong red outline, white tube, solid bulb.
  drawRedCircle(cx, top + 8, 9);
  drawRedCircle(cx, top + 8, 8);
  drawBoldRedLine(cx - 8, top + 8, cx - 8, top + 26, 3);
  drawBoldRedLine(cx + 8, top + 8, cx + 8, top + 26, 3);
  fillRedCircle(cx, top + 26, 11);
  for (int y = top + 8; y <= top + 22; ++y) {
    for (int x = cx - 3; x <= cx + 3; ++x) setWhitePixel(x, y);
  }
}

void drawHumidityIcon(int cx, int top) {
  // Upright solid water drop: pointed top and rounded lower bowl.
  const int baseY = top + 1;
  for (int row = 0; row <= 38; ++row) {
    int halfWidth;
    if (row <= 17) {
      halfWidth = (row * 14) / 17;
    } else {
      const int dy = row - 17;
      halfWidth = static_cast<int>(sqrt(14 * 14 - min(dy, 14) * min(dy, 14)));
    }
    for (int x = -halfWidth; x <= halfWidth; ++x) {
      setRedPixel(cx + x, baseY + row);
    }
  }
}

void drawBrightnessIcon(int cx, int top) {
  // Reference-style brightness icon: ring with four rounded directional nodes.
  const int cy = top + 13;
  drawBoldRedLine(cx - 14, cy, cx + 14, cy, 3);
  drawBoldRedLine(cx, cy - 14, cx, cy + 14, 3);
  fillRedCircle(cx - 20, cy, 5);
  fillRedCircle(cx + 20, cy, 5);
  fillRedCircle(cx, cy - 20, 5);
  fillRedCircle(cx, cy + 20, 5);
  drawRedCircle(cx, cy, 16);
  drawRedCircle(cx, cy, 15);
  drawRedCircle(cx, cy, 14);
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

void drawTitle() {
  // ESP32 + 環境監測, all rendered as bold 20px MingLiU bitmaps.
  const int startX = 73;
  const int ascii[] = {13, 14, 15, 3, 2}; // E S P 3 2
  for (int i = 0; i < 5; ++i) drawKai20Ascii(ascii[i], startX + i * 14, 0);
  const int cjk[] = {17, 18, 19, 20}; // 環 境 監 測
  for (int i = 0; i < 4; ++i) drawKai20(cjk[i], startX + 70 + i * 20, 0);
}

void drawLabel(int centerX, int firstGlyph, int secondGlyph) {
  drawKai20(firstGlyph, centerX - 20, 63);
  drawKai20(secondGlyph, centerX, 63);
}

void drawReading(int centerX, bool valid, int value, bool temperature) {
  if (!valid) {
    for (int i = 0; i < 3; ++i) drawKai20Ascii(10, centerX - 21 + i * 14, 94);
    return;
  }
  char digits[8];
  snprintf(digits, sizeof(digits), "%d", value);
  const int count = static_cast<int>(strlen(digits));
  int width = count * 14 + 14;
  if (temperature) width += 28; // degree + C
  int x = centerX - width / 2;
  for (int i = 0; i < count; ++i) drawKai20Ascii(digits[i] - '0', x + i * 14, 94);
  x += count * 14;
  if (temperature) {
    drawKai20Ascii(16, x, 94); // °
    drawKai20Ascii(12, x + 14, 94); // C
  } else {
    drawKai20Ascii(11, x, 94); // %
  }
}

void drawScreen(bool tempValid, int temperature, bool humidityValid, int humidity,
                bool lightValid, int brightness) {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  const int centers[3] = {49, 148, 247};
  drawTitle();
  drawThermometerIcon(centers[0], 22);
  drawHumidityIcon(centers[1], 22);
  drawBrightnessIcon(centers[2], 22);

  drawLabel(centers[0], 21, 22); // 溫度
  drawLabel(centers[1], 23, 22); // 濕度
  drawLabel(centers[2], 24, 22); // 亮度

  drawReading(centers[0], tempValid, temperature, true);  // °C
  drawReading(centers[1], humidityValid, humidity, false); // %
  drawReading(centers[2], lightValid, brightness, false);   // %

  // Black separators match the reference and stop before the title area.
  for (int y = 20; y < 123; ++y) {
    setBlackPixel(98, y);
    setBlackPixel(197, y);
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

void setup() {
  Serial.begin(115200);
  pinMode(34, INPUT); // GPIO34 is input-only; BUSY must not use INPUT_PULLUP.
  analogReadResolution(12);
  delay(100);
  Serial.println("37_epaper_dht: temperature / humidity / brightness");
  updateDisplay();
  lastUpdate = millis();
}

void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    updateDisplay();
    lastUpdate = millis();
  }
  delay(100);
}
