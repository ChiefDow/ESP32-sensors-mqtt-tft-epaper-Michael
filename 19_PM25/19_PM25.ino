#include <WiFi.h>//WiFi
#include <HTTPClient.h>//瀏覽器
#include <ArduinoJson.h>//請先安裝ArduinoJson程式庫
//OLED DISPLAY
#include "Wire.h"
#include "U8g2lib.h"

char ssid[] = "YOUR_WIFI_SSID"; //請修改為您連線的網路名稱
char password[] = "YOUR_WIFI_PASSWORD"; //請修改為您連線的網路密碼
char url[] = "https://data.moenv.gov.tw/api/v2/aqx_p_02?api_key=YOUR_API_KEY&limit=100"; //讀取的網址及授權密碼
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  //OLED 螢幕解析度為128*64

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("開始連線到無線網路SSID:");
  Serial.println(ssid);
  //1.設定WiFi模式
  WiFi.mode(WIFI_STA);
  //2.啟動WiFi連線
  WiFi.begin(ssid, password);
  //3.檢查連線狀態
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    //OLED中文字型顯示
    u8g2.begin();                                //初始化
    u8g2.enableUTF8Print();                      //啟用 UTF8字集
    u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
    u8g2.setFontPosTop();//座標從上開始

  }
  Serial.println("連線完成");
}

void loop() {
  //4.啟動網頁連線
  Serial.println("啟動網頁連線");
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("httpCode=");
  Serial.println(httpCode);
  //5.檢查網頁連線是否正常
  if (httpCode == HTTP_CODE_OK) {
    //6.取得網頁內容
    String payload = http.getString();
    //Serial.print("payload=");
    
    //Serial.println(payload);
    //JSON格式解析
    DynamicJsonDocument AQIJson(payload.length() * 2); //宣告一個JSON文件，名稱為AQIJson
    deserializeJson(AQIJson, payload);//解析網頁內容payload為JSON格式，存放在AQIJson內
    for (int i = 0; i < AQIJson.size(); i++) {
	  // 瀏覽records內的所有紀錄，直到找到site=="橋頭"
      if (AQIJson[i]["site"] == "鳳山") {        
        String AQI = AQIJson[i]["pm25"];
        Serial.println("鳳山 PM2.5=" + AQI);
        
        //將資料顯示在OLED螢幕上
		    u8g2.clearBuffer();   //清除液晶螢幕
        //第一列
        u8g2.setCursor(0, 5);
        u8g2.print("勞動部高屏澎東署");   
        //第二列
        u8g2.setCursor(0, 25);
        u8g2.print("鳳山區空氣品質");
        //第三列
        u8g2.setCursor(0, 45);
        u8g2.print("PM2.5= " + AQI + " ug");
        //輸出到OLED
        u8g2.sendBuffer();   //傳送到液晶螢幕
        delay(1000);

        //return;
        break;
        

      }
    }
  }
  http.end();
  delay(30000);

  
}
