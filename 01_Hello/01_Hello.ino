//我的ARDUINO第一個程式
void setup() { 
  //初始化
Serial.begin(115200);
}
void loop() {
  //重複執行
Serial.println("GREEN");
delay(3000);
Serial.println("YELLOW");
delay(3000);
Serial.println("RED");
delay(3000);  
}
