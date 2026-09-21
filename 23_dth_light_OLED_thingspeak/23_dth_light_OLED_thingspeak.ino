#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <SimpleDHT.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* THINGSPEAK_API_KEY = "YOUR_THINGSPEAK_API_KEY";
const unsigned long UPLOAD_INTERVAL = 15000;

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

unsigned long lastUpload = 0;
byte temperature = 0;
byte humidity = 0;
int lightRaw = 0;
int brightness = 0;
int dhtError = -1;
String networkStatus = "WiFi:CONNECTING";
String uploadStatus = "Upload:WAIT";

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
  if (fillWidth > 0) {
    oled.drawBox(x + 1, y + 1, fillWidth, height - 2);
  }
}

void drawScreen() {
  oled.clearBuffer();

  oled.setFont(u8g2_font_unifont_t_chinese1);
  oled.drawFrame(0, 0, 64, 32);
  oled.drawFrame(64, 0, 64, 32);
  oled.drawFrame(0, 32, 128, 32);

  drawThermometer(3, 7);
  oled.setCursor(20, 4);
  oled.print("溫度");
  oled.setCursor(20, 19);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(temperature);
    oled.print(" C");
  } else {
    oled.print("Error");
  }

  drawDrop(67, 7);
  oled.setCursor(84, 4);
  oled.print("濕度");
  oled.setCursor(84, 19);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(humidity);
    oled.print(" %");
  } else {
    oled.print("Error");
  }

  drawSun(3, 39);
  oled.setCursor(22, 35);
  oled.print("亮度");
  oled.setCursor(60, 35);
  oled.print(brightness);
  oled.print("%");

  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(4, 53);
  oled.print(networkStatus);
  oled.print(" ");
  oled.print(uploadStatus);
  drawProgressBar(86, 52, 38, 8, brightness);

  oled.sendBuffer();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  networkStatus = "WiFi:CONNECTING";
  uploadStatus = "Upload:WAIT";
  drawScreen();

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    networkStatus = "WiFi:OK";
    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    networkStatus = "WiFi:ERROR";
    Serial.println("Wi-Fi connection failed");
  }
  drawScreen();
}

void uploadToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    networkStatus = "WiFi:ERROR";
    uploadStatus = "Upload:SKIP";
    connectWiFi();
    return;
  }

  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=";
  url += THINGSPEAK_API_KEY;
  url += "&field1=";
  url += String(temperature);
  url += "&field2=";
  url += String(humidity);
  url += "&field3=";
  url += String(brightness);

  Serial.println("Uploading to ThingSpeak...");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Brightness: ");
  Serial.print(brightness);
  Serial.println(" %");

  http.begin(url);
  int httpCode = http.GET();
  String response = http.getString();
  http.end();

  Serial.print("ThingSpeak HTTP code: ");
  Serial.println(httpCode);
  Serial.print("ThingSpeak response: ");
  Serial.println(response);

  if (httpCode == HTTP_CODE_OK && response.toInt() > 0) {
    uploadStatus = "Upload:OK";
  } else {
    uploadStatus = "Upload:ERROR";
  }
  drawScreen();
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  lightRaw = analogRead(LIGHT_PIN);
  brightness = map(lightRaw, 4095, 0, 0, 100);
  brightness = constrain(brightness, 0, 100);
}

void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);

  Wire.begin(21, 22);
  oled.begin();
  oled.enableUTF8Print();
  oled.setFontPosTop();

  readSensors();
  connectWiFi();
  lastUpload = millis() - UPLOAD_INTERVAL;
}

void loop() {
  readSensors();
  drawScreen();

  if (millis() - lastUpload >= UPLOAD_INTERVAL) {
    uploadToThingSpeak();
    lastUpload = millis();
  }

  delay(1000);
}
