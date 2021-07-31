#include <Arduino.h>
#include "WiFi.h"
#include "WifiCredentials.h"

#define SENSOR1 33
#define SENSOR2 32
#define SENSOR3 35
#define SENSOR4 34

#define PUMP1 23
#define PUMP2 22
#define PUMP3 21
#define PUMP4 19

#define WIFI_TIMEOUT_MS 20000

void connectToWifi(){
  Serial.printf("Connecting to WiFi '%s'", WIFI_NETWORK);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

  unsigned long startWifi = millis();

  while(WiFi.status() != WL_CONNECTED && millis() - startWifi < WIFI_TIMEOUT_MS){
    Serial.print(".");
    delay(250);
  }

  if(WiFi.status() != WL_CONNECTED){
    Serial.println(" ! failed !");
    // deep sleep?!
  } else {
    Serial.print(" connected ");
    Serial.println(WiFi.localIP());
  }
}

void setup() {
  Serial.begin(9600);
  connectToWifi();

  pinMode(SENSOR1, INPUT);
  pinMode(SENSOR2, INPUT);
  pinMode(SENSOR3, INPUT);
  pinMode(SENSOR4, INPUT);

  pinMode(PUMP1, OUTPUT);
  pinMode(PUMP2, OUTPUT);
  pinMode(PUMP3, OUTPUT);
  pinMode(PUMP4, OUTPUT);

  digitalWrite(PUMP1, HIGH);
  digitalWrite(PUMP2, HIGH);
  digitalWrite(PUMP3, HIGH);
  digitalWrite(PUMP4, HIGH);
}


void processSensor(uint8_t logical, uint8_t readPin, uint8_t writePin){
  float value = analogRead(readPin);

  Serial.printf("%d (%d) = %f ", logical, readPin, value);

  if(value > 3000){
    digitalWrite(writePin, HIGH);
  } else {
    digitalWrite(writePin, LOW);
  }
}

void loop() {
  processSensor(1, SENSOR1, PUMP1);
  processSensor(2, SENSOR2, PUMP2);
  processSensor(3, SENSOR3, PUMP3);
  processSensor(4, SENSOR4, PUMP4);

  Serial.println();
  delay(750);
}