#include <SPI.h>
#include <TFT_22_ILI9225.h>

const uint8_t TFT_CS = 5;
const uint8_t TFT_RST = 26;
const uint8_t TFT_RS = 27;
const uint8_t TFT_SDI = 23;
const uint8_t TFT_CLK = 18;
const uint8_t TFT_LED = 25;

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED);

void setup() {
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  tft.begin();
  tft.setOrientation(1);
  tft.clear();
  tft.setFont(Terminal12x16);
  tft.drawText(55, 100, "HELLO", COLOR_WHITE);
}

void loop() {
}
