/* --------------------------------------
  Methane Sensing Clinic Integrated Code version 1

  Read data from MH-741A Methane Sensor via I2C, and write to a CSV file on an SD card

  If currently calibrated, concentration value ranges from 0 to 10000 with 10000 being 100% by volume

  The circuit:
   SD card attached to SPI bus as follows:
 ** DI - pin 11
 ** DO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
   MH-741A Methane Sensor wires connected as follows:
 ** RED - 5V
 ** BLACK - GND
 ** YELLOW - SCL
 ** BROWN - SDA

   --------------------------------------
*/

#include <Arduino.h>
#include "Adafruit_SHT31.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// SD Card Variables
File dataFile; // data file to write methane data to
String fileName = "mdata.csv"; // NAME OF FILE (before .csv extension) MUST BE 8 CHARACTERS OR FEWER
int timeDelay = 5000; // time between data entries in milliseconds

// Methane Sensor Variables
const int methaneAddress = 0x55; // Address and commands are from datasheet
const int gasConcCommand = 0x96; // Gas concentration
const int calibrZeroCommand = 0xA0; // Calibrate zero point of methane sensor
const int calibrSpanCommand = 0xAA; // Calibrate span point of methane sensor

// Temperature Sensor Variables
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Salinity Sensor Variables


unsigned long startTime;

// Setup
void setup() {
  Wire.begin();

  Serial.begin(9600);

  // DISABLE PULL UP RESISTORS
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);

  // Initialize components
  initSD();
  initTempSensor();


  //calibrateZero();
  //calibrateSpan(1500); // KEEP COMMENTED OUT IF NOT CALIBRATING (should make it to only calibrate through serial input)

  startTime = millis();
}

// Run continuously
void loop() {
  // Read values
  int currentTime = (millis() - startTime) / 1000
  int methaneReading = readMethane();
  int temperatureReading = readTemperature();
  int salinityReading = readSalinity();

  // Log values
  int data[2] = {currentTime, methaneReading};
  logData(data);
  logToSerial(data);

  // Wait
  delay(timeDelay);
}



/* ----- READING FUNCTIONS ----- */


// Read temperature value from SHT31 sensor, return float in degrees C
int readTemperature() {
  float t = sht31.readTemperature();

  if (! isnan(t)) {  // check if 'is not a number'
    return t;
  } else { 
    Serial.println("Failed to read temperature");
    return 9999; // return unreasonable number if failed to read
  }
}

// Read salinity value from EC sensor, return integer in [unit?]
int readSalinity() {
  return 0;
}

// Read the methane value from the sensor, returns computed methane value (if calibrated, in %Vol * 100)
int readMethane() {
  int reading;
  int highReading;
  int lowReading;

  Wire.beginTransmission(byte(methaneAddress));
  Wire.write(byte(gasConcCommand));
  for (int i = 0; i < 8; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - gasConcCommand));
  Wire.endTransmission();

  delay(70); // Not sure if this is needed

  Wire.requestFrom(methaneAddress, 10); // Request ten bytes

  int i = 0;
  while (Wire.available()) {
    reading = Wire.read(); 
    if (i == 5) {
      highReading = reading;
    } else if (i == 6) {
      lowReading = reading;
    i++;
  }
  Wire.endTransmission();
  return highReading * 256 + lowReading;
}



/* ----- CALIBRATION FUNCTIONS ----- */


// Calibrate the zero point of the sensor (I'm assuming you just run this command in a zero methane environment)
// Planning to have this be triggered by serial command
void calibrateZero() {
  Wire.beginTransmission(byte(methaneAddress));
  Wire.write(byte(calibrZeroCommand));
  for (int i = 0; i < 8; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - calibrZeroCommand));
  Wire.endTransmission();
}

// Calibrate current value of methane
// Planning to have this be triggered by serial command
void calibrateSpan(unsigned int range) {
  // Split range into two bytes
  int highByte = range / 256;
  int lowByte = range % 256;

  int checksum = calibrSpanCommand + highByte + lowByte;

  // Send "calibrate span point" command
  Wire.beginTransmission(byte(methaneAddress));
  Wire.write(byte(calibrSpanCommand));

  Wire.write(byte(highByte));
  Wire.write(byte(lowByte));

  for (int i = 0; i < 6; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - (checksum % 256)));
  Wire.endTransmission();

}



/* ----- INITIALIZATION FUNCTIONS ----- */


// Initialize the SD Card and open CSV file
void initSD() {
  Serial.println("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    while (1); // Stop code from proceeding
  }
  Serial.println("SD initialization complete.");

  dataFile = SD.open(fileName, FILE_WRITE);

  if (!dataFile) {
    Serial.println("Failed to open file!");
    while (1);
  }
  Serial.println("File opened succesfully.");
  dataFile.close();
}

// Initialize the temperature sensor, stop program if not working (maybe final code shouldn't entirely stop the code)
void initTempSensor() {
  Serial.println("Initializing SHT31 Temperature Sensor:");
  if (! sht31.begin(0x44)) 
  {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  Serial.println("SHT31 connected successfully.");
}



/* ----- WRITING FUNCTIONS ----- */


// Write a data entry to CSV file
// data array should take format: {time(s), methane reading (%Vol * 100)}
void logData(int data[2]) {
  // Not sure if this function should open and close the file each time, but it does for now.
  dataFile = SD.open(fileName, FILE_WRITE);

  dataFile.print(data[0]); // Log the time in seconds
  dataFile.print(",");
  dataFile.println(data[1]); // Log value of methane given by sensor

  dataFile.close();
  
  Serial.println("New data logged!");
  Serial.println();
}

// Write current methane data to serial monitor
// data array should take format: {time(s), methane reading (%Vol * 100)}
void logToSerial(int data[2]) {
  char time[15];
  sprintf(time, "Time: %.02d:%.02d", int(data[0] / 60), int(data[0] % 60));
  
  Serial.println(time);

  Serial.print("Methane Concentration (%Vol * 100): ");
  Serial.println(data[1]);

  Serial.println();
}
