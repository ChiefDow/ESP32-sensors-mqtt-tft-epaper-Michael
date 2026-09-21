#include <SimpleDHT.h>
#include "Wire.h"
#include "U8g2lib.h"  
#include <ESP32Servo.h>

//宣告DTH接腳
int pinDHT11 = 19;  
SimpleDHT11 dht11(pinDHT11);

//宣告OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  //OLED 螢幕解析度為128*64

//宣告LED燈腳位
int pinREDLED = 15;
int pinGREENLED = 2;
int pinYELLOWLED = 4;
int pinBLUELED = 16;

//宣告GLED燈腳位
int R = 25;
int G = 26;
int B = 27;

//宣告Buzzer腳位
int buzzer = 17;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始

  pinMode(pinREDLED,OUTPUT);
  pinMode(pinGREENLED,OUTPUT);
  pinMode(pinYELLOWLED,OUTPUT);
  pinMode(pinBLUELED,OUTPUT);
  pinMode(R,OUTPUT);
  pinMode(G,OUTPUT);
  pinMode(B,OUTPUT);

}

void loop() 
{
  // put your main code here, to run repeatedly:
  //讀取溫溼度
  Serial.println("=================================");
  Serial.println("Sample DHT11...");
  
  //byte=>0~255範圍的整數
  byte temperature = 0;
  byte humidity = 0;

  //如果讀取錯誤,就印出錯誤訊息並返回return,返回開始的地方
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) 
  {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    
    u8g2.clearBuffer();   //清除液晶螢幕
    
    u8g2.setCursor(0, 5);
    u8g2.print("DHT11異常/未開啟");   

    u8g2.setCursor(0, 25);
    u8g2.print("Code:");u8g2.print(SimpleDHTErrCode(err));
    
    u8g2.setCursor(0, 45);
    u8g2.print("Duration:");u8g2.println(SimpleDHTErrDuration(err)); 
        //delay(1000);
    u8g2.sendBuffer();    //傳送到液晶螢幕
    return;
  }
  
  Serial.println("Sample OK: " + (String)temperature +" *C, " +(String)humidity +" H");

  u8g2.clearBuffer();                                 //清除液晶螢幕
  u8g2.setCursor(0, 5);                               //移動游標
  u8g2.print("勞動部高屏澎東署");                      //寫入文字

  u8g2.setCursor(0, 25);                              //移動游標
  u8g2.print("溫度：" + (String)temperature + " C");  //寫入文字

  u8g2.setCursor(0, 45);                              //移動游標
  u8g2.print("濕度：" + (String)humidity + " %");     //寫入文字

  u8g2.sendBuffer();  //送到螢幕顯示
  
  //溫度感測大於28度時,亮黃燈,否則亮藍燈
  if (temperature >= 28)
  {
    digitalWrite(pinYELLOWLED,HIGH);
    digitalWrite(pinBLUELED,LOW);

    //u8g2.clearBuffer();                        //顯示前清除螢幕
    u8g2.setCursor(0, 5);                        //移動游標
    u8g2.print("勞動部高屏澎東署");               //寫入文字

    u8g2.setCursor(0, 25);                       //移動游標
    u8g2.print("溫度：" + (String)temperature + " C" + "*過溫*");     //寫入文字

    u8g2.sendBuffer();  //送到螢幕顯示
  }
  else
  {
    digitalWrite(pinYELLOWLED,LOW);
    digitalWrite(pinBLUELED,HIGH);
  }

  //濕度感測大於60度時,亮紅燈,否則亮綠燈
  if (humidity <= 60) 
  {
    digitalWrite(pinGREENLED,HIGH);
    digitalWrite(pinREDLED,LOW);
    
    analogWrite(R,0);
    analogWrite(G,255);
    analogWrite(B,0);
    
  }
  else 
  {
    digitalWrite(pinGREENLED,LOW);
    digitalWrite(pinREDLED,HIGH);
    //u8g2.clearBuffer();                        //顯示前清除螢幕
    u8g2.setCursor(0, 5);                      //移動游標
    u8g2.print("勞動部高屏澎東署");             //寫入文字
    
    u8g2.setCursor(0, 45);                    //移動游標
    u8g2.print("濕度：" + (String)humidity + " %" + "*過濕*");  //寫入文字

    u8g2.sendBuffer();  //送到螢幕顯示

  }
  
  //======================================
  if(humidity >= 61 and humidity < 70)
  {
   //Y
   analogWrite(R,255);
   analogWrite(G,255);
   analogWrite(B,0);
    
  }

  if(humidity >= 71 and humidity < 80)
  {
   //O
   analogWrite(R,255);
   analogWrite(G,187);
   analogWrite(B,0);
   
  }
  
  if(humidity >= 81 and humidity < 90)
  {
   //靛
   analogWrite(R,153);
   analogWrite(G,0);
   analogWrite(B,255);
   
  }
  
  if(humidity >= 91 and humidity < 100)
  {
   //R
   analogWrite(R,255);
   analogWrite(G,0);
   analogWrite(B,0);
  }
  
  if(humidity > 70)
  {
    tone(buzzer, 262, 500); // C(Do)
    tone(buzzer, 330, 500); // E(Mi)
    
    delay(500);
  }
  else{noTone(buzzer);}

  delay(1000);

}
