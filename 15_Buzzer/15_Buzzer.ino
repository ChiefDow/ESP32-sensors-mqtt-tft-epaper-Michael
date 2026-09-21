#include <ESP32Servo.h>
int buzzer= 17;
void setup() {
 Serial.begin(115200);
}
void loop() {
 tone(buzzer, 392, 500); // G(So)
 tone(buzzer, 330, 500); // E(Mi)
 tone(buzzer, 330, 500); // E(Mi)
 delay(500);
 tone(buzzer, 349, 500); // F(Fa)
 tone(buzzer, 294, 500); // D(Re)
 tone(buzzer, 294, 500); // D(Re)
 delay(500);
 tone(buzzer, 262, 500); // C(Do)
 tone(buzzer, 294, 500); // D(Re)
 tone(buzzer, 330, 500); // E(Mi)
 tone(buzzer, 349, 500); // F(Fa)
 tone(buzzer, 392, 500); // G(So)
 tone(buzzer, 392, 500); // G(So)
 tone(buzzer, 392, 500); // G(So)
 delay(3000);
}