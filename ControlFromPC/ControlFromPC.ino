//從電腦端用C#寫APP程式控制
void setup() 
{
   Serial.begin(115200);
   pinMode(15,OUTPUT);
   pinMode(2,OUTPUT);

}

char c;
void loop()
{
  if(Serial.available())
  {
    c = Serial.read();
    Serial.println(c);

  }

  if(c == '1')
     digitalWrite(15,HIGH);
  else if(c == '0')
     digitalWrite(15,LOW);
  


}
