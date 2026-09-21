

int G = 15;

void setup() {
  // put your setup code here, to run once:
  pinMode(G,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  analogWrite(G,13);  //假設5%亮度 0~255=0~100%  x/255=5/100 約為12.75 之後依此類堆
  delay(1000);
  analogWrite(G,77);  //30%亮度
  delay(1000);
  analogWrite(G,154); //60%亮度
  delay(1000);       
  analogWrite(G,231); //90%亮度
  delay(1000);        
  analogWrite(G,235); //100%亮度
  delay(5000);
}
