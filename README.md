# ESP32 Sensors MQTT TFT E-Paper

Arduino/ESP32 experiments and working examples covering sensors, Wi‑Fi, MQTT, TFT displays, OLED displays, and Waveshare e-paper modules.

## Contents

- `40_epaper_mqtt/` – ESP32 e-paper sensor display with MQTT publishing.
- `37_epaper_dht/` and `36_epaper/` – e-paper and DHT sensor experiments.
- `26_MQTT/`, `27_MQTT_ctrl/`, `33_ili_mqtt/`, and `34_ili_MQTT_ctrl/` – MQTT and TFT control examples.
- `21_WeatherOLED/`, `22_DHT_light_OLED/`, and related folders – OLED sensor and weather examples.
- `libraries/` – libraries used by the sketches.
- `成果照片/` and project reports – supporting project documentation and media.

## Hardware and software

- ESP32 development boards
- DHT/AHT temperature and humidity sensors
- TFT/OLED displays and Waveshare e-paper displays
- Arduino IDE or Arduino CLI with the ESP32 board package
- MQTT broker for MQTT examples

## Getting started

1. Open the desired `.ino` sketch in Arduino IDE.
2. Install the required libraries listed by the compiler.
3. Select the matching ESP32 board and serial port.
4. Set local Wi‑Fi and service credentials in the sketch before uploading.
5. Compile and upload the sketch.

Credentials are intentionally not stored in this public repository. Replace placeholder values such as `YOUR_WIFI_SSID`, `YOUR_WIFI_PASSWORD`, and `YOUR_API_KEY` locally before uploading.

## Notes

This repository contains iterative learning and prototype sketches. Pin assignments, display drivers, MQTT topics, and sensor wiring vary by folder; check each sketch before connecting hardware.

## Project showcase

The static showcase site is in [`docs/`](docs/). It can be published with GitHub Pages using the `main` branch and `/docs` folder.
