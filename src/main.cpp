#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Raspberry Pi Pico initialized!");
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
