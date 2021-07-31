#include <Arduino.h>

#include "WiFi.h"
#include "WifiSecrets.h"

#include "ThingSpeak.h"
#include "ThingSpeakSecrets.h"

#define SENSOR1 33
#define SENSOR2 32
#define SENSOR3 35
#define SENSOR4 34

#define PUMP1 23
#define PUMP2 22
#define PUMP3 21
#define PUMP4 19

#define WIFI_TIMEOUT_MS 20000

WiFiClient wifiClient;

void connectToWifi()
{
  Serial.printf("Connecting to WiFi '%s'", WIFI_NETWORK);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

  unsigned long startWifi = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startWifi < WIFI_TIMEOUT_MS)
  {
    Serial.print(".");
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(" ! failed !");
    sleep(60);
  }
  else
  {
    Serial.print(" connected ");
    Serial.println(WiFi.localIP());
  }
}

void setup()
{
  Serial.begin(9600);

  connectToWifi();
  ThingSpeak.begin(wifiClient);

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

int processSensor(uint8_t field, uint8_t readPin, uint8_t writePin)
{
  float value = analogRead(readPin);

  Serial.printf("%d (%d) = %f ", field, readPin, value);
  ThingSpeak.setField(field, value);

  if (value > 3000)
  {
    digitalWrite(writePin, HIGH);
    return 0;
  }
  else
  {
    digitalWrite(writePin, LOW);
    return 1;
  }
}

void loop()
{
  String status = "";

  if (processSensor(1, SENSOR1, PUMP1))
  {
    status += "PUMPING 1";
  }
  if (processSensor(2, SENSOR2, PUMP2))
  {
    if (status.length() > 0)
    {
      status += ", ";
    }
    status += "PUMPING 2";
  }
  if (processSensor(3, SENSOR3, PUMP3))
  {
    if (status.length() > 0)
    {
      status += ", ";
    }
    status += "PUMPING 3";
  }
  if (processSensor(4, SENSOR4, PUMP4))
  {
    if (status.length() > 0)
    {
      status += ", ";
    }
    status += "PUMPING 4";
  }

  if (status.length() == 0)
  {
    status = "-- OFF --";
  }

  Serial.println(status);
  ThingSpeak.setStatus(status);

  ThingSpeak.setField(8, WiFi.RSSI());

  ThingSpeak.writeFields(TP_CHANNEL, TP_WRITE_API_KEY);

  delay(10000);
}