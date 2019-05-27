#include "SdsDustSensor.h"
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

int rxPin = D6;
int txPin = D7;
SdsDustSensor sds(rxPin, txPin);
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7);

void setup() {
  Serial.begin(9600);
  sds.begin();
  lcd.begin(20, 4);
  lcd.setBacklightPin(3, POSITIVE);
  lcd.backlight();

  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
  Serial.println(sds.setCustomWorkingPeriod(1).toString()); // ensures sensor has continuous working period - default but not recommended
}

void loop() {
  PmResult pm = sds.readPm();

  if (pm.isOk()) {
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
  } else {
    // notice that loop delay is set to 0.5s and some reads are not available
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }

  delay(500);
}
