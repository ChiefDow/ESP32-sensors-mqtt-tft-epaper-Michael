#include <WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>
#include <Wire.h>
#include <U8g2lib.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "mdow/class305/data";
const char* MQTT_CTRL_TOPIC = "mdow/class305/ctrl/#";
const char* GLED_CTRL_TOPIC = "mdow/class305/ctrl/gled";
const char* YLED_CTRL_TOPIC = "mdow/class305/ctrl/yled";
const char* RLED_CTRL_TOPIC = "mdow/class305/ctrl/rled";
const unsigned long MQTT_INTERVAL = 10000;

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;

SimpleDHT11 dht11(DHT_PIN);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

unsigned long lastPublish = 0;
byte temperature = 0;
byte humidity = 0;
int lightRaw = 0;
int brightness = 0;
int dhtError = SimpleDHTErrSuccess;
String clientId;
String mqttStatus = "MQTT:WAIT";
String lastCommand = "";
unsigned long commandDisplayUntil = 0;

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
  oled.setCursor(20, 4);
  oled.print("TEMP");
  oled.setCursor(20, 17);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(temperature);
    oled.print(" C");
  } else {
    oled.print("ERROR");
  }

  drawDrop(67, 6);
  oled.setCursor(84, 4);
  oled.print("HUM");
  oled.setCursor(84, 17);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(humidity);
    oled.print(" %");
  } else {
    oled.print("ERROR");
  }

  drawSun(3, 39);
  oled.setCursor(22, 36);
  oled.print("LIGHT");
  oled.setCursor(60, 36);
  oled.print(brightness);
  oled.print("%");
  drawProgressBar(86, 38, 38, 8, brightness);
  oled.setCursor(4, 53);
  if (millis() < commandDisplayUntil) {
    oled.print("CMD: ");
    oled.print(lastCommand);
  } else {
    oled.print(mqttStatus);
  }
  oled.sendBuffer();
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, NULL);
  lightRaw = analogRead(LIGHT_PIN);
  brightness = map(lightRaw, 4095, 0, 0, 100);
  brightness = constrain(brightness, 0, 100);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttStatus = "WIFI...";
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
    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
    mqttStatus = "WIFI OK";
  } else {
    Serial.println("Wi-Fi connection failed");
    mqttStatus = "WIFI ERR";
  }
  drawScreen();
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  mqttStatus = "MQTT...";
  drawScreen();

  Serial.print("Connecting to MQTT as ");
  Serial.println(clientId);
  if (mqttClient.connect(clientId.c_str())) {
    mqttStatus = "MQTT OK";
    Serial.println("MQTT connected");
    if (mqttClient.subscribe(MQTT_CTRL_TOPIC)) {
      Serial.print("Subscribed to: ");
      Serial.println(MQTT_CTRL_TOPIC);
    } else {
      Serial.println("MQTT subscribe failed");
    }
  } else {
    mqttStatus = "MQTT ERR";
    Serial.print("MQTT connection failed, state=");
    Serial.println(mqttClient.state());
  }
  drawScreen();
}

bool parseOnOff(String value, bool& state) {
  value.trim();
  value.toLowerCase();
  if (value.startsWith("\"")) value = value.substring(1);
  if (value.endsWith("\"")) value.remove(value.length() - 1);
  value.trim();
  if (value == "on" || value == "true" || value == "1") {
    state = true;
    return true;
  }
  if (value == "off" || value == "false" || value == "0") {
    state = false;
    return true;
  }
  return false;
}

bool applyStateCommand(const String& message, int pin, const char* label) {
  const String stateKey = "\"state\"";
  int keyPosition = message.indexOf(stateKey);
  if (keyPosition < 0) return false;
  int colonPosition = message.indexOf(':', keyPosition + stateKey.length());
  if (colonPosition < 0) return false;
  int valueEnd = message.indexOf(',', colonPosition + 1);
  if (valueEnd < 0) valueEnd = message.indexOf('}', colonPosition + 1);
  if (valueEnd < 0) valueEnd = message.length();

  bool state = false;
  String value = message.substring(colonPosition + 1, valueEnd);
  if (!parseOnOff(value, state)) return false;
  digitalWrite(pin, state ? HIGH : LOW);
  lastCommand = String(label) + (state ? " ON" : " OFF");
  commandDisplayUntil = millis() + 1000;
  Serial.print("Control: ");
  Serial.println(lastCommand);
  drawScreen();
  return true;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String receivedTopic = topic;
  int targetPin = -1;
  const char* targetLabel = "";
  if (receivedTopic == GLED_CTRL_TOPIC) {
    targetPin = GREEN_LED_PIN;
    targetLabel = "LIGHT";
  } else if (receivedTopic == YLED_CTRL_TOPIC) {
    targetPin = YELLOW_LED_PIN;
    targetLabel = "FAN";
  } else if (receivedTopic == RLED_CTRL_TOPIC) {
    targetPin = RED_LED_PIN;
    targetLabel = "DEHUM";
  } else {
    return;
  }

  String message;
  message.reserve(length + 1);
  for (unsigned int index = 0; index < length; index++) message += static_cast<char>(payload[index]);
  Serial.print("MQTT control received [");
  Serial.print(receivedTopic);
  Serial.print("]: ");
  Serial.println(message);

  if (!applyStateCommand(message, targetPin, targetLabel)) {
    lastCommand = "INVALID";
    commandDisplayUntil = millis() + 1000;
    Serial.println("Invalid control command");
    drawScreen();
  }
}
void publishData() {
  if (!mqttClient.connected()) return;
  if (dhtError != SimpleDHTErrSuccess) {
    mqttStatus = "DHT ERR";
    Serial.print("DHT11 read failed, error=");
    Serial.println(dhtError);
    drawScreen();
    return;
  }

  String payload = String("{\"temp\":") + temperature +
                   ",\"humi\":" + humidity +
                   ",\"light\":" + brightness + "}";
  bool published = mqttClient.publish(MQTT_TOPIC, payload.c_str());
  mqttStatus = published ? "PUB OK" : "PUB ERR";
  Serial.print("MQTT topic: ");
  Serial.println(MQTT_TOPIC);
  Serial.print("MQTT payload: ");
  Serial.println(payload);
  drawScreen();
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  Wire.begin(21, 22);
  oled.begin();
  oled.setFontPosTop();

  uint64_t chipId = ESP.getEfuseMac();
  char clientIdBuffer[24];
  snprintf(clientIdBuffer, sizeof(clientIdBuffer), "esp32-%04X%08X",
           static_cast<unsigned int>(chipId >> 32),
           static_cast<unsigned int>(chipId & 0xFFFFFFFF));
  clientId = clientIdBuffer;

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  readSensors();
  drawScreen();
  connectWiFi();
  connectMQTT();
  lastPublish = millis() - MQTT_INTERVAL;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  readSensors();
  drawScreen();
  if (millis() - lastPublish >= MQTT_INTERVAL) {
    publishData();
    lastPublish = millis();
  }
  delay(1000);
}
