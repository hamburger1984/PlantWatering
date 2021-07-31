#include <Arduino.h>

#include "WiFi.h"
#include "WifiSecrets.h"

#include "ThingSpeak.h"
#include "ThingSpeakSecrets.h"

#define SENSOR1 33
#define SENSOR2 32
#define SENSOR3 35
#define SENSOR4 34

#define PUMP_THRESHOLD 2900

#define PUMP1 23
#define PUMP2 22
#define PUMP3 21
#define PUMP4 19

#define DEEP_SLEEP_MINUTES 5
#define DEEP_SLEEP_DELAY_MS 1000

#define PUMPING_DELAY_MS 2000

#define WIFI_TIMEOUT_MS 20000

WiFiClient wifiClient;

void goToDeepSleep()
{
  Serial.print("Going to sleep.");
  delay(DEEP_SLEEP_DELAY_MS);

  Serial.println(" bye");
  delay(DEEP_SLEEP_DELAY_MS); // trying to let the wifi transmit data..

  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_MINUTES * 60 * 1000 * 1000);
  esp_deep_sleep_start();
}

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
    goToDeepSleep();
  }
  else
  {
    Serial.print(" connected ");
    Serial.println(WiFi.localIP());
  }
}

bool processSensor(uint8_t field, uint8_t readPin, uint8_t writePin)
{
  float value = analogRead(readPin);

  Serial.printf("%d (%d) = %f ", field, readPin, value);
  ThingSpeak.setField(field, value);

  if (value < PUMP_THRESHOLD)
  {
    digitalWrite(writePin, HIGH);
    return false;
  }
  else
  {
    digitalWrite(writePin, LOW);
    return true;
  }
}

bool measureAndPump(bool sendUpdate)
{
  String status = "";
  bool result = false;

  if (processSensor(1, SENSOR1, PUMP1))
  {
    result = true;
    status += "PUMPING 1";
  }
  if (processSensor(2, SENSOR2, PUMP2))
  {
    if (result)
    {
      status += ", ";
    }
    result = true;
    status += "PUMPING 2";
  }
  if (processSensor(3, SENSOR3, PUMP3))
  {
    if (result)
    {
      status += ", ";
    }
    result = true;
    status += "PUMPING 3";
  }
  if (processSensor(4, SENSOR4, PUMP4))
  {
    if (result)
    {
      status += ", ";
    }
    result = true;
    status += "PUMPING 4";
  }

  if (!result)
  {
    status = "-- OFF --";
  }

  Serial.println(status);
  ThingSpeak.setStatus(status);

  ThingSpeak.setField(8, WiFi.RSSI());

  if (sendUpdate)
  {
    Serial.println("Send update.");
    ThingSpeak.writeFields(TP_CHANNEL, TP_WRITE_API_KEY);
  }
  return result;
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

  if (!measureAndPump(true))
  {
    goToDeepSleep();
  }
}

int count = 0;
void loop()
{
  delay(PUMPING_DELAY_MS);

  count++;
  if (!measureAndPump(count % 10 == 0))
  {
    Serial.println("Pumping done? re-check.");
    delay(PUMPING_DELAY_MS);
    if (!measureAndPump(true))
    {
      goToDeepSleep();
    }
  }
}