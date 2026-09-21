// 人體紅外線感測器(PIR)讀取程式

const int PIR_PIN = 18;   // PIR感測器接在D18
const int RED_LED = 15;
const int GREEN_LED = 4;

void setup()
{
    pinMode(PIR_PIN, INPUT);
    pinMode(RED_LED,OUTPUT);
    pinMode(GREEN_LED,OUTPUT);

    Serial.begin(115200);

    Serial.println("PIR Sensor Start");
}

void loop()
{
    int pirState = digitalRead(PIR_PIN);

    Serial.print("PIR State = ");

    if (pirState == HIGH)
    {
        Serial.println("Motion Detected");
        digitalWrite(GREEN_LED,LOW);
        digitalWrite(RED_LED,HIGH);
        delay(15000);
        
    }
    else
    {
        Serial.println("No Motion");
        digitalWrite(RED_LED,LOW);
        digitalWrite(GREEN_LED,HIGH);
    }

    delay(100);   // 每1秒讀取一次
}