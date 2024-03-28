/*
  Author: Michelle Frolio and Jeremy Sher
  Description: This code is to test the temperature and EC sensor for the methane sensor clinic
*/

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {
  // Begin Serial Monitor to Display Data
  Serial.begin(9600);


  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) 
  {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

}

void loop() {
  float t = sht31.readTemperature();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.println("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  } 

  delay(1000);

}
