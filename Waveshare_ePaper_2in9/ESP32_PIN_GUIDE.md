# Waveshare 2.9 吋三色電子紙 V4：ESP32 接線

適用：296 x 128、SPI、三色（黑／白／紅）模組，官方範例目錄 `Arduino/epd2in9b_V4`。

## 建議接線

| 電子紙腳位 | ESP32 Dev Module | 說明 |
|---|---:|---|
| VCC | 3V3 | 建議使用 3.3 V 供電 |
| GND | GND | 共地 |
| DIN / MOSI | GPIO23 | VSPI MOSI |
| CLK / SCK | GPIO18 | VSPI SCK |
| CS | GPIO27 | SPI 片選 |
| DC | GPIO26 | 資料／命令選擇 |
| RST | GPIO25 | 電子紙重置 |
| BUSY | GPIO34 | 電子紙忙碌狀態輸入；GPIO34 只能輸入 |

MISO（GPIO19）不必接，因為此電子紙主要使用 ESP32 輸出資料；若模組排針有 MISO 腳位，可保持未接。

## 與目前專案的 GPIO 配置

- DHT11：GPIO14，保留。
- 光敏電阻：GPIO33，保留。
- 綠色 LED：GPIO15，保留。
- 黃色 LED：GPIO2，保留。
- 紅色 LED：GPIO4，保留。
- 頁面按鈕：GPIO0，保留。
- 電子紙新增：GPIO5、16、17、18、23、27。

上述配置沒有和目前專案的主要腳位衝突。GPIO34 沒有內建上拉／下拉電阻，但 BUSY 是電子紙模組輸出的狀態訊號，因此使用 `pinMode(BUSY_PIN, INPUT)` 即可，不要使用 `INPUT_PULLUP`。電子紙接入後，原本的 ILI9225 TFT 應先停用或移除，避免同時佔用 SPI 與顯示控制流程。

## 官方範例

先用官方範例單獨測試電子紙，再合併 MQTT 與感測程式。官方範例原本使用 Arduino 腳位 7、8、9、10，整合 ESP32 時必須改成上表 GPIO。

下載檔案：`E-Paper_code.zip`

範例：`source/Arduino/epd2in9b_V4/epd2in9b_V4.ino`

注意：官方 V4 範例的 `epdif.h` 內要改為：

```cpp
#define RST_PIN  25
#define DC_PIN   26
#define CS_PIN   27
#define BUSY_PIN 34
```

SPI 時脈建議先維持官方範例的 2 MHz，確認穩定後再調整。電子紙刷新速度慢，請勿像 TFT 一樣每 2 秒整頁刷新；建議至少間隔 180 秒，並在刷新後進入 Sleep。
