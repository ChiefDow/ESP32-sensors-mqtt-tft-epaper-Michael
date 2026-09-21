// ESP32-WROVER RGB彩虹燈
// 共陰極RGB LED

#define RED_PIN     25
#define GREEN_PIN   26
#define BLUE_PIN    27

#define RED_CH      0
#define GREEN_CH    1
#define BLUE_CH     2

void setup()
{
    ledcSetup(RED_CH, 5000, 8);
    ledcSetup(GREEN_CH, 5000, 8);
    ledcSetup(BLUE_CH, 5000, 8);

    ledcAttachPin(RED_PIN, RED_CH);
    ledcAttachPin(GREEN_PIN, GREEN_CH);
    ledcAttachPin(BLUE_PIN, BLUE_CH);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    ledcWrite(RED_CH, r);
    ledcWrite(GREEN_CH, g);
    ledcWrite(BLUE_CH, b);
}

void loop()
{
    int r, g, b;

    // 紅 → 黃
    for(int i=0;i<=255;i++)
    {
        r = 255;
        g = i;
        b = 0;

        setRGB(r,g,b);
        delay(10);
    }

    // 黃 → 綠
    for(int i=255;i>=0;i--)
    {
        r = i;
        g = 255;
        b = 0;

        setRGB(r,g,b);
        delay(10);
    }

    // 綠 → 青
    for(int i=0;i<=255;i++)
    {
        r = 0;
        g = 255;
        b = i;

        setRGB(r,g,b);
        delay(10);
    }

    // 青 → 藍
    for(int i=255;i>=0;i--)
    {
        r = 0;
        g = i;
        b = 255;

        setRGB(r,g,b);
        delay(10);
    }

    // 藍 → 紫
    for(int i=0;i<=255;i++)
    {
        r = i;
        g = 0;
        b = 255;

        setRGB(r,g,b);
        delay(10);
    }

    // 紫 → 紅
    for(int i=255;i>=0;i--)
    {
        r = 255;
        g = 0;
        b = i;

        setRGB(r,g,b);
        delay(10);
    }
}