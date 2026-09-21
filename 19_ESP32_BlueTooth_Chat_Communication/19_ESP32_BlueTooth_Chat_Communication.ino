#include <BluetoothSerial.h>
BluetoothSerial BT;
int RedLED = 15;
int GreenLED = 2;
int YellowLED = 4;


void setup() {
  pinMode(RedLED,OUTPUT);//紅燈
  pinMode(GreenLED,OUTPUT);//綠燈
  pinMode(YellowLED,OUTPUT);//黃燈

  Serial.begin(115200);
  BT.begin("DOU_BT24");//自己設定ESP32的藍芽名稱
}

void loop() {
  //檢查序列監控視窗是否有輸入資料
  String Sdata="";
  String BTdata="";
  while (Serial.available()) 
  {
    //讀取序列資料
    char Schar = Serial.read();//一次讀一個字元
    Sdata = Sdata + Schar;
  }

  if (Sdata!="") BT.println(Sdata);

  //檢查藍芽內是否有資料
  while (BT.available()) {
    //讀取藍芽資料
    char BTchar = BT.read();
    BTdata = BTdata + BTchar;
  }

  if (BTdata!="") Serial.println(BTdata);
  
  if (BTdata == "0") digitalWrite(RedLED,LOW);
  if (BTdata == "1") digitalWrite(RedLED,HIGH);
  
  if (BTdata == "2") digitalWrite(GreenLED,LOW);
  if (BTdata == "3") digitalWrite(GreenLED,HIGH);

  if (BTdata == "4") digitalWrite(YellowLED,LOW);
  if (BTdata == "5") digitalWrite(YellowLED,HIGH);

  delay(10);
}
