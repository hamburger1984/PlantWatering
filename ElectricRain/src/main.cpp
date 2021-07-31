#include <Arduino.h>

#include "WiFi.h"
#include "WifiSecrets.h"

#include "ThingSpeak.h"
#include "ThingSpeakSecrets.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

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

// UTC
#define PUMPING_HOURS_START 2
#define PUMPING_HOURS_END 6

#define PUMPING_DELAY_MS 2000

#define NTP_TIMEOUT_MS 10000
#define WIFI_TIMEOUT_MS 20000

WiFiClient wifiClient;

WiFiUDP wifiUdpClient;
NTPClient timeClient(wifiUdpClient);

uint16_t sensor1, sensor2, sensor3, sensor4;
bool pump1, pump2, pump3, pump4;

void goToDeepSleep(uint64_t sleepMinutes)
{
  Serial.printf("Sleeping for %llu minutes.", sleepMinutes);
  delay(DEEP_SLEEP_DELAY_MS);

  Serial.println(" bye");
  delay(DEEP_SLEEP_DELAY_MS); // trying to let the wifi transmit data..

  esp_sleep_enable_timer_wakeup(sleepMinutes * 60 * 1000 * 1000);
  esp_deep_sleep_start();
}

void shutdownOutsidePumpingHours()
{
  timeClient.update();

  // calculate how long we want to sleep to increase the chances to
  // wake up at a time we're allowed to pump.

  // 0 ..... PUMPING_HOURS_START ###### PUMPING_HOURS_END ...... 23
  // 0 ### PUMPING_HOURS_END ........... PUMPING_HOURS_START ### 23

  uint8_t hour = timeClient.getHours();
  uint8_t minute = timeClient.getMinutes();

  if (PUMPING_HOURS_START < PUMPING_HOURS_END)
  {

    // we're not in the pumping window
    if (hour < PUMPING_HOURS_START || hour > PUMPING_HOURS_END)
    {
      Serial.printf("Outside pumping hours @%d:%d (allowed to pump from %d:00 to %d:59)\n", hour, minute, PUMPING_HOURS_START, PUMPING_HOURS_END);
      goToDeepSleep(((24 + PUMPING_HOURS_START - hour) % 24) * 60 - minute);
    }
  }
  else
  {

    // we're not in the pumping window
    if (hour < PUMPING_HOURS_START && hour > PUMPING_HOURS_END)
    {
      Serial.printf("Outside pumping hours @%d:%d (allowed to pump until %d:00 and after %d:59)\n", hour, minute, PUMPING_HOURS_END, PUMPING_HOURS_START);
      goToDeepSleep(((24 + PUMPING_HOURS_START - hour) % 24) * 60 - minute);
    }
  }
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
    goToDeepSleep(DEEP_SLEEP_MINUTES);
  }
  else
  {
    Serial.print(" connected ");
    Serial.println(WiFi.localIP());
  }
}

void ensureTime()
{
  timeClient.begin();

  unsigned long startNtp = millis();

  Serial.print("Fetching time");
  while (!timeClient.update() && millis() - startNtp < NTP_TIMEOUT_MS)
  {
    Serial.print(".");
    delay(250);
  }

  if (!timeClient.update())
  {
    Serial.println(" ! failed !");
    goToDeepSleep(DEEP_SLEEP_MINUTES);
  }
  else
  {
    Serial.print(" got ");
    Serial.println(timeClient.getFormattedTime());
  }
}

uint16_t readSensor(uint8_t field, uint8_t sensorPin)
{
  uint16_t value = (uint16_t)analogRead(sensorPin);

  Serial.printf("%d (%d) = %d .. ", field, sensorPin, value);

  return value;
}

bool updatePump(uint8_t pumpPin, uint16_t value)
{
  if (value < PUMP_THRESHOLD)
  {
    digitalWrite(pumpPin, HIGH);
    return false;
  }
  else
  {
    digitalWrite(pumpPin, LOW);
    return true;
  }
}

bool measureAndPump(bool sendUpdate)
{
  sensor1 = readSensor(1, SENSOR1);
  sensor2 = readSensor(2, SENSOR2);
  sensor3 = readSensor(3, SENSOR3);
  sensor4 = readSensor(4, SENSOR4);

  pump1 = updatePump(PUMP1, sensor1);
  pump2 = updatePump(PUMP2, sensor2);
  pump3 = updatePump(PUMP3, sensor3);
  pump4 = updatePump(PUMP4, sensor4);

  if (sendUpdate)
  {
    String status = "";
    if (pump1)
    {
      status += "PUMPING 1 ";
    }
    if (pump2)
    {
      status += "PUMPING 2 ";
    }
    if (pump3)
    {
      status += "PUMPING 3 ";
    }
    if (pump4)
    {
      status += "PUMPING 4 ";
    }
    if (status.length() == 0)
    {
      status = "-- OFF -- ";
    }

    Serial.println(status);
    ThingSpeak.setStatus(status.substring(0, status.length() - 1));

    ThingSpeak.setField(1, sensor1);
    ThingSpeak.setField(2, sensor2);
    ThingSpeak.setField(3, sensor3);
    ThingSpeak.setField(4, sensor4);
    ThingSpeak.setField(8, WiFi.RSSI());

    Serial.println("Send update.");
    ThingSpeak.writeFields(TP_CHANNEL, TP_WRITE_API_KEY);
  }
  else
  {
    Serial.println();
  }

  return pump1 || pump2 || pump3 || pump4;
}

void setup()
{
  Serial.begin(9600);

  connectToWifi();
  ensureTime();

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

  shutdownOutsidePumpingHours();

  if (!measureAndPump(true))
  {
    goToDeepSleep(DEEP_SLEEP_MINUTES);
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
      goToDeepSleep(DEEP_SLEEP_MINUTES);
    }
  }

  if (count % 20 == 0)
  {
    shutdownOutsidePumpingHours();
  }
}