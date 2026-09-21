#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AHT10.h>
#include <SimpleTimer.h>
#include <time.h>

// BLE Credentials
const char *service_name = "esp32s3";//自己改名稱
const char *pop = "esp32";           //自己改密碼

// NTP Server Details
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;    //taiwan GMT+8*3600sec
const int   daylightOffset_sec = 0;

// OLED Display
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define I2Cdisplay_SDA 18    //GPIO 18
#define I2Cdisplay_SCL 19    //GPIO 19
TwoWire I2Cdisplay = TwoWire(1);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//AHT10
uint8_t readStatus = 0;
AHT10 myAHT10(AHT10_ADDRESS_0X38); //I2C PIN 8 9
#define I2C_SDA 18     //GPIO 18
#define I2C_SCL 19     //GPIO 19
TwoWire I2CAHT10 = TwoWire(0);
float temp=0.0;
float humi=0.0;

// Screens
int displayScreenNum = 0;
int displayScreenNumMax = 2;

unsigned long lastTimer = 0;
unsigned long timerDelay = 15000;       //15 seconds

unsigned char temperature_icon[] ={
  0b00000001, 0b11000000, //        ###      
  0b00000011, 0b11100000, //       #####     
  0b00000111, 0b00100000, //      ###  #     
  0b00000111, 0b11100000, //      ######     
  0b00000111, 0b00100000, //      ###  #     
  0b00000111, 0b11100000, //      ######     
  0b00000111, 0b00100000, //      ###  #     
  0b00000111, 0b11100000, //      ######     
  0b00000111, 0b00100000, //      ###  #     
  0b00001111, 0b11110000, //     ########    
  0b00011111, 0b11111000, //    ##########   
  0b00011111, 0b11111000, //    ##########   
  0b00011111, 0b11111000, //    ##########   
  0b00011111, 0b11111000, //    ##########   
  0b00001111, 0b11110000, //     ########    
  0b00000111, 0b11100000, //      ######     
};

unsigned char humidity_icon[] ={
  0b00000000, 0b00000000, //                 
  0b00000001, 0b10000000, //        ##       
  0b00000011, 0b11000000, //       ####      
  0b00000111, 0b11100000, //      ######     
  0b00001111, 0b11110000, //     ########    
  0b00001111, 0b11110000, //     ########    
  0b00011111, 0b11111000, //    ##########   
  0b00011111, 0b11011000, //    ####### ##   
  0b00111111, 0b10011100, //   #######  ###  
  0b00111111, 0b10011100, //   #######  ###  
  0b00111111, 0b00011100, //   ######   ###  
  0b00011110, 0b00111000, //    ####   ###   
  0b00011111, 0b11111000, //    ##########   
  0b00001111, 0b11110000, //     ########    
  0b00000011, 0b11000000, //       ####      
  0b00000000, 0b00000000, //                 
};

// Create display marker for each screen
void displayIndicator(int displayNumber) {
  int xCoordinates[3] = {54, 64, 74};   //44,54,64,74,84
  for (int i =0; i<3; i++) {
    if (i == displayNumber) {
      display.fillCircle(xCoordinates[i], 60, 2, WHITE);
    }
    else {
      display.drawCircle(xCoordinates[i], 60, 2, WHITE);
    }
  }
}

//SCREEN NUMBER 0: DATE AND TIME
void displayLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  //GET DATE
  //Get full weekday name
  char weekDay[10];
  strftime(weekDay, sizeof(weekDay), "%a", &timeinfo);
  //Get day of month
  char dayMonth[4];
  strftime(dayMonth, sizeof(dayMonth), "%d", &timeinfo);
  //Get abbreviated month name
  char monthName[5];
  strftime(monthName, sizeof(monthName), "%b", &timeinfo);
  //Get year
  char year[6];
  strftime(year, sizeof(year), "%Y", &timeinfo);

  //GET TIME
  //Get hour (12 hour format)
  /*char hour[4];
  strftime(hour, sizeof(hour), "%I", &timeinfo);*/
  
  //Get hour (24 hour format)
  char hour[4];
  strftime(hour, sizeof(hour), "%H", &timeinfo);
  //Get minute
  char minute[4];
  strftime(minute, sizeof(minute), "%M", &timeinfo);

  //Display Date and Time on OLED display
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(3);
  display.setCursor(19,5);
  display.print(hour);
  display.print(":");
  display.print(minute);
  display.setTextSize(1);
  display.setCursor(16,40);
  display.print(weekDay);
  display.print(", ");
  display.print(dayMonth);
  display.print(" ");
  display.print(monthName);
  display.print(" ");
  display.print(year);  
  displayIndicator(displayScreenNum);  
  display.display();  
}

// SCREEN NUMBER 1: TEMPERATURE
void displayTemperature(){  
  display.clearDisplay();
  display.setTextSize(2);
  display.drawBitmap(15, 5, temperature_icon, 16, 16 ,1);
  display.setCursor(35, 5);  
  temp = myAHT10.readTemperature();
  display.print(myAHT10.readTemperature());
  display.cp437(true);
  display.setTextSize(1);
  display.print(" ");
  display.write(248);
  display.print("C");
  display.setCursor(0, 34);
  display.setTextSize(1);
  display.print("Humidity: ");
  display.print(myAHT10.readHumidity());
  display.print(" %");  
  displayIndicator(displayScreenNum);  
  display.display();  
}

// SCREEN NUMBER 2: HUMIDITY
void displayHumidity(){  
  display.clearDisplay();
  display.setTextSize(2);
  display.drawBitmap(15, 5, humidity_icon, 16, 16 ,1);
  display.setCursor(35, 5);
  humi = myAHT10.readHumidity();
  display.print(humi);
  display.print(" %");
  display.setCursor(0, 34);
  display.setTextSize(1);
  display.print("Temperature: ");
  display.print(myAHT10.readTemperature());
  display.cp437(true);
  display.print(" ");
  display.write(248);
  display.print("C");
  displayIndicator(displayScreenNum);
  display.display();  
}

// Display the right screen accordingly to the displayScreenNum
void updateScreen() {
  //colorWipe(strip.Color(0, 0, 0), 1, LED_COUNT);
  if (displayScreenNum == 0){
    displayLocalTime();
  }
  else if (displayScreenNum == 1) {
    displayTemperature();
  }
  else if (displayScreenNum ==2){
    displayHumidity();
  }  
}

#if CONFIG_IDF_TARGET_ESP32C3
static int gpio_reset = 9;
//static int led_pin = 18;
//static int relay_pin = 10;
//bool led_pin_state = true;
//bool relay_pin_state = true;
#else
//GPIO for virtual device ESP32S2
static int gpio_reset = 0;
static int led_pin = 2;
static int relay_pin = 4;  //Relay S
bool led_pin_state = true;
bool relay_pin_state = true;
#endif

bool wifi_connected = 0;

SimpleTimer Timer;

//declare devices
static TemperatureSensor temperature("Temperature");
static TemperatureSensor humidity("Humidity");
static Switch button("LED", &led_pin);
static Switch relay("Switch", &relay_pin);

void sysProvEvent(arduino_event_t *sys_event)
{
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32S2
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n", service_name, pop);
      printQR(service_name, pop, "softap");
#else
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
      printQR(service_name, pop, "ble");
#endif
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.printf("\nConnected to Wi-Fi!\n");
      wifi_connected = 1;
      delay(500);
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV: {
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("\tSSID : ");
        Serial.println((const char *) sys_event->event_info.prov_cred_recv.ssid);
        Serial.print("\tPassword : ");
        Serial.println((char const *) sys_event->event_info.prov_cred_recv.password);
        break;
      } 
  }
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
  const char *device_name = device->getDeviceName();  
  const char *param_name = param->getParamName();

  if (strcmp(device_name, "LED") == 0)
  {
    if (strcmp(param_name, "Power") == 0) {
      Serial.printf("Received value = %s for %s - %s\n", val.val.b ? "true" : "false", device_name, param_name);
      led_pin_state = val.val.b;
      (led_pin_state == false) ? digitalWrite(led_pin, LOW) : digitalWrite(led_pin, HIGH);
      param->updateAndReport(val);
    }
  }else if (strcmp(device_name, "Switch") == 0)
  {
    if(strcmp(param_name, "Power") == 0) {
        Serial.printf("Received value = %s for %s - %s\n", val.val.b? "true" : "false", device_name, param_name);
        relay_pin_state = val.val.b;
        (relay_pin_state == false) ? digitalWrite(relay_pin, LOW) : digitalWrite(relay_pin, HIGH);
        param->updateAndReport(val);
     }
  }
}

void setup()
{
  Serial.begin(115200);
  
  Wire.begin(I2C_SDA,I2C_SCL);
  Wire.begin(I2Cdisplay_SDA, I2Cdisplay_SCL); 

  // Initialize OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
   
  //init AHT10 sensor
  myAHT10.begin();
     
  pinMode(gpio_reset, INPUT);
  pinMode(led_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  digitalWrite(led_pin,LOW);
  digitalWrite(relay_pin,LOW);
  
  //declare node
  Node my_node;
  my_node = RMaker.initNode("RMakerS2 AHT OLED");

  button.addCb(write_callback);
  relay.addCb(write_callback);

  //Add devices
  my_node.addDevice(temperature);
  my_node.addDevice(humidity);
  my_node.addDevice(button);
  my_node.addDevice(relay);

  //This is optional
  RMaker.enableOTA(OTA_USING_PARAMS);
  //If you want to enable scheduling, set time zone for your region using setTimeZone().
  //The list of available values are provided here https://rainmaker.espressif.com/docs/time-service.html
  //RMaker.setTimeZone("Asia/Shanghai");
  //Alternatively, enable the Timezone service and let the phone apps set the appropriate timezone
  RMaker.enableTZService();
  RMaker.enableSchedule();

  Serial.printf("\nStarting ESP-RainMaker\n");
  RMaker.start();

  // Timer for Sending Sensor Data
  Timer.setInterval(60000);      //60 sec

  WiFi.onEvent(sysProvEvent);

#if CONFIG_IDF_TARGET_ESP32S2
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#else
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#endif

}

void loop()
{
  if (Timer.isReady() && wifi_connected) {                    
    Serial.println("Sending Sensor Data");

  //String getReadings()
  //float temp, humi;
  float temp=0.0;
  float humi=0.0;
  temp = myAHT10.readTemperature();
  humi = myAHT10.readHumidity();
  delay(100);
  String message = "temperature: " + String(temp) + " ℃ \n";
  message += "humidity: " + String (humi) + " % \n";     
    
    Serial.print("Temperature: "); 
    Serial.println(temp);
    Serial.print("Humidity: "); 
    Serial.println(humi);
    
    temperature.updateAndReportParam("Temperature", temp);
    humidity.updateAndReportParam("Temperature", humi);
    Timer.reset();                        
  }

  // Read GPIO0 (external button to reset device
  if (digitalRead(gpio_reset) == LOW) { //Push button pressed
    Serial.printf("Reset Button Pressed!\n");
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW) delay(50);
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      // If key pressed for more than 10secs, reset all
      Serial.printf("Reset to factory.\n");
      wifi_connected = 0;
      RMakerFactoryReset(2);
    } else if ((endTime - startTime) > 3000) {
      Serial.printf("Reset Wi-Fi.\n");
      wifi_connected = 0;
      // If key pressed for more than 3secs, but less than 10, reset Wi-Fi
      RMakerWiFiReset(2);
    }
  }
  // Change screen every 15 seconds (timerDelay variable)
  if ((millis() - lastTimer) > timerDelay) {
    updateScreen();
    Serial.println(displayScreenNum);
    if(displayScreenNum < displayScreenNumMax) {
      displayScreenNum++;
    }
    else {
      displayScreenNum = 0;
    }
    lastTimer = millis();
  }
  delay(100);
}
