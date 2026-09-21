#include <SPI.h>
#include "TFT_22_ILI9225.h"

const uint8_t TFT_SCK = 18;
const uint8_t TFT_MISO = 19;
const uint8_t TFT_MOSI = 23;
const uint8_t TFT_CS = 5;
const uint8_t TFT_RST = 26;
const uint8_t TFT_DC = 27;
const uint8_t TFT_LED = 25;

TFT_22_ILI9225 tft(TFT_RST, TFT_DC, TFT_CS, TFT_LED);

const uint16_t colors[] = {
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_WHITE,
  COLOR_BLACK
};

void setup() {
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);

  tft.begin();
  tft.setOrientation(0);
  tft.setBacklight(true);
}

void loop() {
  for (uint8_t index = 0; index < 5; index++) {
    tft.fillRectangle(0, 0, 175, 219, colors[index]);
    delay(3000);
  }
}