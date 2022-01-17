#include "Arduino.h"

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Start");
}

void loop() {
  delay(10000);
}
