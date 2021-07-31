#include <Arduino.h>

#define SENSOR1 33
#define SENSOR2 32
#define SENSOR3 35
#define SENSOR4 34

void setup() {
  Serial.begin(9600);

  pinMode(SENSOR1, INPUT);
  pinMode(SENSOR2, INPUT);
  pinMode(SENSOR3, INPUT);
  pinMode(SENSOR4, INPUT);
}


void processSensor(uint8_t logical, uint8_t readPin){
  float value = analogRead(readPin);

  Serial.printf("%d (%d) = %f ", logical, readPin, value);
}

void loop() {
  processSensor(1, SENSOR1);
  processSensor(2, SENSOR2);
  processSensor(3, SENSOR3);
  processSensor(4, SENSOR4);

  Serial.println();
  delay(750);
}