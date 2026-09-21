const int lightPin = 36;  // GPIO36 / ADC1_CH0

const int LED1 = 15;      // 第一顆LED
const int LED2 = 2;       // 第二顆LED
const int LED3 = 4;       // 第三顆LED

void setup()
{
    Serial.begin(115200);

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);

    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
}

void loop()
{
    int value = analogRead(lightPin);

    // ADC(0~4095)轉換成亮度百分比(100~0)
    value = map(value, 0, 4095, 100, 0);

    Serial.print("Light = ");
    Serial.println(value);

    // 預設全部熄滅
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);

    // 多段式小夜燈控制
    if (value <= 75)
    {
        // 最暗：三顆全亮
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, HIGH);
    }
    else if (value <= 80)
    {
        // 較暗：亮兩顆
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
    }
    else if (value <= 85)
    {
        // 微暗：亮一顆
        digitalWrite(LED1, HIGH);
    }

    delay(100);   // 每秒10次
}