#include <ESP32Servo.h>

#include "Wire.h"
#include "U8g2lib.h"

int Trig =12;//發出聲波腳位
int Echo =14;//接收聲波腳位
int buzzer= 17;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  //OLED 螢幕解析度為128*64

void setup(){
  Serial.begin(115200);
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始

  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);

}

void loop() {
  digitalWrite(Trig, LOW); //先關閉
  delayMicroseconds(5);
  digitalWrite(Trig, HIGH);//啟動超音波
  delayMicroseconds(10);  
  digitalWrite(Trig, LOW); //關閉
  float EchoTime = pulseIn(Echo, HIGH); //計算傳回時間
  float CMValue = EchoTime / 29.4 / 2; //將時間轉換成距離
  Serial.println(CMValue);
  delay(50);
  
//小於10公分時警報作動及OLED顯示接近的距離
if(CMValue <= 10.0)
{
  tone(buzzer, 1000, 500);delay(500);
  
  u8g2.clearBuffer();   //清除液晶螢幕
    
  u8g2.setCursor(0, 5);
  u8g2.print("物體接近距離");   

  u8g2.setCursor(0, 25);
  u8g2.print(CMValue);u8g2.print("公分");

  u8g2.sendBuffer();    //傳送到液晶螢幕
  return;
}
else{noTone(buzzer);}

}

