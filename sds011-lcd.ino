#include "SdsDustSensor.h"
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ESP8266WiFi.h>
#include "secrets.h"

#define LED 2

const int rxPin = D6;
const int txPin = D7;
const int no2Pin = A0;
const float no2Resistor = 22000;
SdsDustSensor sds(rxPin, txPin);
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7);

WiFiClient wifiClient;

Adafruit_MQTT_Client mqtt(&wifiClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME,
                          AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish feedPm10 =
    Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pm10");
Adafruit_MQTT_Publish feedPm25 =
    Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pm25");
Adafruit_MQTT_Publish feedNo2 =
    Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/no2");

void setup()
{
  Serial.begin(9600);

  pinMode(LED, OUTPUT);
  pinMode(no2Pin, INPUT);
  // Turn LED off until MQTT is connected.
  digitalWrite(LED, HIGH);

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
  Serial.println(" connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

float readNo2() {
  int no2Raw = analogRead(no2Pin);
  Serial.print("Raw no2 value:");
  Serial.println(no2Raw);
  float no2Resistance = no2Resistor * ((1023.0 / no2Raw) - 1.0) / 100;
  Serial.print("No2 resistance:");
  Serial.println(no2Resistance);
  // https://uk-air.defra.gov.uk/assets/documents/reports/cat06/0502160851_Conversion_Factors_Between_ppb_and.pdf
  return no2Resistance / 1.19125;
}

void loop()
{
  connectMQTT();
  PmResult pm = sds.readPm();

  const float no2 = readNo2();
  lcd.setCursor(0, 2);
  lcd.print("NO2: ");
  lcd.setCursor(7, 2);
  lcd.print(readNo2());

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

    if (!feedPm10.publish(pm.pm10, 4))
    {
      Serial.println(F("Failed to publish PM10 to MQTT."));
    }
    if (!feedPm25.publish(pm.pm25, 4))
    {
      Serial.println(F("Failed to publish PM25 to MQTT."));
    }
    if (!feedNo2.publish(no2, 4))
    {
      Serial.println(F("Failed to publish NO2 to MQTT."));
    }
  }
  else
  {
    // notice that loop delay is set to 0.5s and some reads are not available
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }

  // ping the server to keep the mqtt connection alive
  if (!mqtt.ping()) {
    mqtt.disconnect();
  }

  delay(500);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void connectMQTT() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  // connect will return 0 for connected
  while ((ret = mqtt.connect()) != 0) {
    digitalWrite(LED, HIGH);
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000); // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  digitalWrite(LED, LOW);
  Serial.println("MQTT Connected!");
}
