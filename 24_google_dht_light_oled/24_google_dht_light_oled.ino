#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SimpleDHT.h>
#include <Wire.h>
#include <U8g2lib.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const String SHEET_ID = "自己的";
const String SHEET_NAME = "data";
const char* GOOGLE_SCRIPT_ID = "自己的";

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const unsigned long UPLOAD_INTERVAL = 10000;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

unsigned long lastUpload = 0;
byte temperature = 0;
byte humidity = 0;
int lightRaw = 0;
int brightness = 0;
int dhtError = SimpleDHTErrSuccess;
String networkStatus = "WiFi:CONNECTING";
String uploadStatus = "WAIT";

void drawThermometer(int x, int y) {
  oled.drawFrame(x + 5, y, 5, 13);
  oled.drawDisc(x + 7, y + 15, 4);
  oled.drawBox(x + 6, y + 5, 3, 10);
  oled.drawLine(x + 10, y + 3, x + 12, y + 3);
  oled.drawLine(x + 10, y + 7, x + 12, y + 7);
  oled.drawLine(x + 10, y + 11, x + 12, y + 11);
}

void drawDrop(int x, int y) {
  oled.drawLine(x + 7, y, x + 2, y + 8);
  oled.drawLine(x + 7, y, x + 12, y + 8);
  oled.drawCircle(x + 7, y + 9, 5);
  oled.drawDisc(x + 7, y + 9, 3);
}

void drawSun(int x, int y) {
  oled.drawCircle(x + 7, y + 7, 4);
  oled.drawLine(x + 7, y, x + 7, y - 3);
  oled.drawLine(x + 7, y + 14, x + 7, y + 17);
  oled.drawLine(x, y + 7, x - 3, y + 7);
  oled.drawLine(x + 14, y + 7, x + 17, y + 7);
  oled.drawLine(x + 2, y + 2, x, y);
  oled.drawLine(x + 12, y + 2, x + 14, y);
  oled.drawLine(x + 2, y + 12, x, y + 14);
  oled.drawLine(x + 12, y + 12, x + 14, y + 14);
}

void drawProgressBar(int x, int y, int width, int height, int percent) {
  oled.drawFrame(x, y, width, height);
  int fillWidth = map(percent, 0, 100, 0, width - 2);
  if (fillWidth > 0) oled.drawBox(x + 1, y + 1, fillWidth, height - 2);
}

void drawScreen() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setFontPosTop();
  oled.drawFrame(0, 0, 64, 31);
  oled.drawFrame(64, 0, 64, 31);
  oled.drawFrame(0, 31, 128, 33);

  drawThermometer(3, 6);
  oled.setCursor(20, 4); oled.print("TEMP");
  oled.setCursor(20, 17);
  if (dhtError == SimpleDHTErrSuccess) { oled.print(temperature); oled.print(" C"); }
  else oled.print("ERROR");

  drawDrop(67, 6);
  oled.setCursor(84, 4); oled.print("HUM");
  oled.setCursor(84, 17);
  if (dhtError == SimpleDHTErrSuccess) { oled.print(humidity); oled.print(" %"); }
  else oled.print("ERROR");

  drawSun(3, 39);
  oled.setCursor(22, 36); oled.print("LIGHT");
  oled.setCursor(60, 36); oled.print(brightness); oled.print("%");
  drawProgressBar(86, 38, 38, 8, brightness);
  oled.setCursor(4, 53); oled.print(networkStatus); oled.print(" "); oled.print(uploadStatus);
  oled.sendBuffer();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  networkStatus = "WIFI..."; uploadStatus = "WAIT"; drawScreen();
  Serial.print("Connecting to Wi-Fi: "); Serial.println(WIFI_SSID);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) { delay(500); Serial.print("."); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    networkStatus = "WIFI OK";
    Serial.print("Wi-Fi connected, IP: "); Serial.println(WiFi.localIP());
  } else { networkStatus = "WIFI ERR"; Serial.println("Wi-Fi connection failed"); }
  drawScreen();
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  lightRaw = analogRead(LIGHT_PIN);
  brightness = map(lightRaw, 4095, 0, 0, 100);
  brightness = constrain(brightness, 0, 100);
}

String urlEncode(const char* message) {
  const char* hex = "0123456789ABCDEF";
  String encoded;
  while (*message != '\0') {
    unsigned char character = static_cast<unsigned char>(*message++);
    if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
        (character >= '0' && character <= '9') || character == '-' || character == '_' ||
        character == '.' || character == '~') encoded += static_cast<char>(character);
    else { encoded += '%'; encoded += hex[character >> 4]; encoded += hex[character & 0x0F]; }
  }
  return encoded;
}

void sendToGoogleSheets(const String& data) {
  if (WiFi.status() != WL_CONNECTED) {
    networkStatus = "WIFI ERR"; uploadStatus = "NO WIFI"; drawScreen(); return;
  }
  WiFiClientSecure sheetClient;
  sheetClient.setInsecure();
  const char* host = "script.google.com";
  uploadStatus = "UPLOADING"; drawScreen(); Serial.println("Uploading to Google Sheet...");
  if (!sheetClient.connect(host, 443)) {
    uploadStatus = "UPLOAD ERR"; drawScreen(); Serial.println("Google Sheet connection failed"); return;
  }
  String url = String("/macros/s/") + GOOGLE_SCRIPT_ID + "/exec?type=insert&dateInclude=1&sheetId=" + SHEET_ID;
  url += "&sheetTag="; url += urlEncode(SHEET_NAME.c_str());
  url += "&data="; url += urlEncode(data.c_str());
  sheetClient.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" +
                    "Accept: */*\r\n" + "Connection: close\r\n\r\n");
  uploadStatus = "UPLOAD ERR";
  unsigned long timeout = millis();
  while (sheetClient.connected() && millis() - timeout < 5000) {
    while (sheetClient.available()) {
      String line = sheetClient.readStringUntil('\n');
      if (line.startsWith("HTTP/1.1 200") || line.startsWith("HTTP/1.0 200")) uploadStatus = "UPLOAD OK";
      timeout = millis();
    }
  }
  sheetClient.stop(); drawScreen();
  Serial.print("Google Sheet status: "); Serial.println(uploadStatus);
}

void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  Wire.begin(21, 22);
  oled.begin(); oled.setFontPosTop();
  readSensors(); drawScreen(); connectWiFi();
  lastUpload = millis() - UPLOAD_INTERVAL;
}

void loop() {
  readSensors(); drawScreen();
  if (millis() - lastUpload >= UPLOAD_INTERVAL) {
    if (dhtError == SimpleDHTErrSuccess) {
      sendToGoogleSheets(String(temperature) + "," + String(humidity) + "," + String(brightness));
    } else {
      uploadStatus = "DHT ERROR"; drawScreen();
      Serial.print("DHT11 read failed, error="); Serial.println(dhtError);
    }
    lastUpload = millis();
  }
  delay(1000);
}


