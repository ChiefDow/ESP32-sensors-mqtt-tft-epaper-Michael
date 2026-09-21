#include <SimpleDHT.h>

// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT11 = 19;           //定義整數值,命名為pinDHT11,設定在第19腳腳位
SimpleDHT11 dht11(pinDHT11); //我有一隻SimpleDHT11規格的溫溼度感測器,命名為dht11(放在第19腳)

int pinREDLED = 15;
int pinGREENLED = 2;
int pinYELLOWLED = 4;
int pinBLUELED = 17;

void setup() 
{
  Serial.begin(115200);
  pinMode(pinREDLED,OUTPUT);
  pinMode(pinGREENLED,OUTPUT);
  pinMode(pinYELLOWLED,OUTPUT);
  pinMode(pinBLUELED,OUTPUT);
}

void loop() 
{
  // start working...
  Serial.println("=================================");
  Serial.println("Sample DHT11...");
  
  // read without samples,byte=>0~255範圍的整數
  byte temperature = 0;
  byte humidity = 0;

  //如果讀取錯誤,就印出錯誤訊息並返回void loop()
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) 
  {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return;
  }
  //顯示溫溼度感測的數據值
  //Serial.println("Sample OK: " + (String)temperature + " *C, " + (String)humidity + " H");  //強制將數值轉型為字串(String)
  Serial.print("Sample OK: ");  //Serial.print是接著印出不換行,Serial.println是印出後換行
  Serial.print((int)temperature); Serial.print(" *C, "); //強制轉型為整數(int)型態
  Serial.print((int)humidity); Serial.println(" H");
  
  //濕度感測大於60度時,亮紅燈,否則亮綠燈
  if (humidity >= 60) 
  {
    digitalWrite(pinREDLED,HIGH);
    digitalWrite(pinGREENLED,LOW);
  }
  else 
  {
    digitalWrite(pinGREENLED,HIGH);
    digitalWrite(pinREDLED,LOW);
  }
  
  //溫度感測大於25度時,亮黃燈,否則亮藍燈
  if (temperature >= 25)
  {
    digitalWrite(pinYELLOWLED,HIGH);
    digitalWrite(pinBLUELED,LOW);
  }
  else
  {
    digitalWrite(pinYELLOWLED,LOW);
    digitalWrite(pinBLUELED,HIGH);
  }
  
  // DHT11 sampling rate is 1HZ.
  delay(1500); //循環延遲1.5秒
}
