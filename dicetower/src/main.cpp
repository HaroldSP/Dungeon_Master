#include <Arduino.h>
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("Starting onboard LED blink...");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // off (active-low)
}

void loop() {
  // Active-low LED on ESP8266: LOW = on, HIGH = off
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
}


