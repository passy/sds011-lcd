#include "SdsDustSensor.h"
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ESP8266WiFi.h>
#include "secrets.h"

int rxPin = D6;
int txPin = D7;
SdsDustSensor sds(rxPin, txPin);
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7);

WiFiClient wifiClient;

Adafruit_MQTT_Client mqtt(&wifiClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME,
                          AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish feedPm10 =
    Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pm10");
Adafruit_MQTT_Publish feedPm25 =
    Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pm25");

void setup()
{
  Serial.begin(9600);
  sds.begin();
  lcd.begin(20, 4);
  lcd.setBacklightPin(3, POSITIVE);
  lcd.backlight();

  Serial.println(sds.queryFirmwareVersion().toString());    // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString());  // ensures sensor is in 'active' reporting mode
  Serial.println(sds.setCustomWorkingPeriod(1).toString()); // ensures sensor has continuous working period - default but not recommended

  // Connect to WiFi access point.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
}

void loop()
{
  PmResult pm = sds.readPm();

  if (pm.isOk())
  {
    Serial.print("PM2.5: ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);

    lcd.home();
    lcd.print("PM2.5: ");
    lcd.setCursor(7, 0);
    lcd.print(pm.pm25);
    lcd.setCursor(0, 1);
    lcd.print("PM10: ");
    lcd.setCursor(7, 1);
    lcd.print(pm.pm10);

    if (!feedPm10.publish(pm.pm10))
    {
      Serial.println(F("Failed to publish PM10 to MQTT."));
    }
    if (!feedPm25.publish(pm.pm25))
    {
      Serial.println(F("Failed to publish PM10 to MQTT."));
    }
  }
  else
  {
    // notice that loop delay is set to 0.5s and some reads are not available
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }

  delay(500);
}
