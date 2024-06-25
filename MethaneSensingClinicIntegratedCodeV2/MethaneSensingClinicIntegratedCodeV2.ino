/* --------------------------------------
  Methane Sensing Clinic Integrated Code version 2

  Read data from:
    - MH-741A Methane Sensor
    - SHT31 Temperature Sensor
    - Electric Conductivity Sensor (for salinity reading)
    - Real Time Clock Module
  Save data to SD Card

   --------------------------------------
*/

#include <Arduino.h>
#include "Adafruit_SHT31.h"
#include <RTClib.h>
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>

/* --- Low Power Mode --- */
int methaneGatePin = 8; //Pin for the gate of the mosfet controlling the power for the methane sensor 
int ECGatePin = 9; //Pin for the gate of the mosfet controlling the power for the EC sensor 
int delayMax = 30000; // 30 seconds Delay
int delayBetweenMeasurementsInMinutes = 1; 

/* --- SD Card Variables --- */
File dataFile; // data file to write methane data to
//String fileName = "mdata.csv"; // NAME OF FILE (before .csv extension) MUST BE 8 CHARACTERS OR FEWER


/* --- Methane Sensor Variables --- */
//const int methaneAddress = 0x55; // Address and commands are from datasheet
//const int gasConcCommand = 0x96; // Gas concentration
//const int calibrZeroCommand = 0xA0; // Calibrate zero point of methane sensor
//const int calibrSpanCommand = 0xAA; // Calibrate span point of methane sensor

/* --- Temperature Sensor Variables --- */
Adafruit_SHT31 sht31 = Adafruit_SHT31();

/* --- Salinity Sensor Variables --- */
const int EC_RE = 6;
const int EC_DE = 2;
const int EC_RO = 5; //was 12 for mega;
const int EC_DI = 3;
//const byte ec[] = {0x01, 0x03, 0x00, 0x15, 0x00, 0x01, 0x95, 0xCE};
//const byte salinity[] = {0x01, 0x03, 0x00, 0x14, 0x00, 0x01, 0xC4, 0x0E};
byte ecValues[8];
SoftwareSerial mod(EC_RO, EC_DI);

/* -- Real Time Clock Module Variables --- */
RTC_DS3231 rtc; // create rtc for the DS3231 RTC module, address is fixed at 0x68
//unsigned int timeArray[6]; // Stores values for Time in format {year, month, day, hour, minute, second}

// Setup
void setup() {
  // Initializing Outputs
  pinMode(methaneGatePin, OUTPUT);
  pinMode(ECGatePin, OUTPUT);
  digitalWrite(methaneGatePin, HIGH);
  digitalWrite(ECGatePin, HIGH);

  Wire.begin();

  //Serial.begin(9600);
  mod.begin(9600);

  // DISABLE PULL UP RESISTORS
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);

  pinMode(EC_RE, OUTPUT);
  pinMode(EC_DE, OUTPUT);

  // Initialize components
  initSD();
  delay(1000);
  initTempSensor();
  delay(10);
  initClockModule();
  delay(10);

  //calibrateZero();
  //calibrateSpan(1500); // KEEP COMMENTED OUT IF NOT CALIBRATING (should make it to only calibrate through serial input)
  //display_freeram();

  // logs 10 values to the SD card to allow user to confirm set up is operating correctly
  for (int i = 0; i <= 10; i++) {
    // Read values
    readTime();
    delay(10);
    readAndLogTemperature();
    delay(10);
    readAndLogEC();
    delay(10);
    readAndLogMethane();
    delay(10);
    logDataNewLine();
    delay(2000); //delay of 2 seconds
  }
}

// Run continuously
void loop() {

  for (int i = 1; i <= (delayBetweenMeasurementsInMinutes*2); i++) {
  delay(delayMax);
  }

  // Read values
  readTime();
  delay(10);
  readAndLogTemperature();
  delay(10);
  readAndLogEC();
  delay(10);
  readAndLogMethane();
  delay(10);
  logDataNewLine();
}

 

/******************************************************************************************/
/* ------------------------------ INITIALIZATION FUNCTIONS ------------------------------ */
/******************************************************************************************/


// Initialize the SD Card and open CSV file
void initSD() {
  const int SD_CS = 4; // chip select (4 for UNO, 53 for MEGA)
  //Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD initialization failed!");
    while (1); // Stop code from proceeding
  }
  //Serial.println("SD initialization complete.");
  
  dataFile = SD.open("mdata.csv", FILE_WRITE);
  
  if (!dataFile) {
    //Serial.println("Failed to open file!");
    while (1);
  }
  //Serial.println("File opened succesfully.");
  dataFile.close();
  
}

// Initialize the temperature sensor, stop program if not working (maybe final code shouldn't entirely stop the code)
void initTempSensor() {
  //Serial.println("Initializing SHT31 Temperature Sensor:");
  if (! sht31.begin(0x44)) 
  {   // Set to 0x45 for alternate i2c addr
    //Serial.println("Couldn't find SHT31");
    while (0) delay(1);
  }
  //Serial.println("SHT31 connected successfully.");
}

// Intialize the real time clock module peripheral
void initClockModule() {
  rtc.begin();
  //Serial.println("RTC Intialized");
}



/***********************************************************************************/
/* ------------------------------ READING FUNCTIONS ------------------------------ */
/***********************************************************************************/


// Read temperature value from SHT31 sensor, return float in degrees C
float readTemperature() {
  float t = sht31.readTemperature();

  if (! isnan(t)) {  // check if 'is not a number'
    return t;
  } else { 
    Serial.println("Failed to read temperature");
    return 9999; // return unreasonable number if failed to read
  }
}

void readAndLogTemperature() {
  float t = sht31.readTemperature();
  float temperatureReading;

  if (! isnan(t)) {  // check if 'is not a number'
    temperatureReading = t;
  } else { 
    //Serial.println("Failed to read temperature");
    temperatureReading = 9999; // return unreasonable number if failed to read
  }
  delay(10);
  logSingleData(String(temperatureReading, 2));
}

// Read soil salinity value from EC sensor, return float in mg/L
float readSalinity() {
  const byte ec[] = {0x01, 0x03, 0x00, 0x15, 0x00, 0x01, 0x95, 0xCE};
  digitalWrite(EC_DE, HIGH);
  digitalWrite(EC_RE, HIGH);
  delay(10);
 
  if (mod.write(ec, sizeof(ec)) == 8)
  {
    digitalWrite(EC_DE, LOW);
    digitalWrite(EC_RE, LOW);
    for (byte i = 0; i < 7; i++)
    {
      ecValues[i] = mod.read();
      //Serial.print(ecValues[i], HEX);
    }
    //Serial.println();
  }

  return soilSalinityConversion(int(ecValues[3]<<8|ecValues[4]));
}

// Read soil salinity value from EC sensor, return float in mg/L
void readAndLogEC() {
  const byte ec[] = {0x01, 0x03, 0x00, 0x15, 0x00, 0x01, 0x95, 0xCE};
  digitalWrite(EC_DE, HIGH);
  digitalWrite(EC_RE, HIGH);
  delay(10);
 
  if (mod.write(ec, sizeof(ec)) == 8)
  {
    digitalWrite(EC_DE, LOW);
    digitalWrite(EC_RE, LOW);
    for (byte i = 0; i < 7; i++)
    {
      ecValues[i] = mod.read();
      //Serial.print(ecValues[i], HEX);
    }
    //Serial.println();
  }

  float ECReading = (int(ecValues[3]<<8|ecValues[4]));
  logSingleData(String(ECReading, 3));
}

// Read the methane value from the sensor, returns computed methane value (if calibrated, in %Vol * 100)
int readMethane() {
  int reading;
  int highReading;
  int lowReading;

  Wire.beginTransmission(byte(0x55));
  Wire.write(byte(0x96));
  for (int i = 0; i < 8; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - 0x96));
  Wire.endTransmission();

  delay(70); // Not sure if this is needed

  Wire.requestFrom(0x55, 10); // Request ten bytes

  int i = 0;
  while (Wire.available()) {
    reading = Wire.read(); 
    //Serial.print(reading);
    if (i == 5) {
      highReading = reading;
    } else if (i == 6) {
      lowReading = reading;
    }
    i++;
  }
  //Serial.println();
  Wire.endTransmission();
  return highReading * 256 + lowReading;
}

// Read the methane value from the sensor, returns computed methane value (if calibrated, in %Vol * 100)
void readAndLogMethane() {
  int reading;
  int highReading;
  int lowReading;
  int methaneReading;

  Wire.beginTransmission(byte(0x55));
  Wire.write(byte(0x96));
  for (int i = 0; i < 8; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - 0x96));
  Wire.endTransmission();

  delay(70); // Not sure if this is needed

  Wire.requestFrom(0x55, 10); // Request ten bytes

  int i = 0;
  while (Wire.available()) {
    reading = Wire.read(); 
    //Serial.print(reading);
    if (i == 5) {
      highReading = reading;
    } else if (i == 6) {
      lowReading = reading;
    }
    i++;
  }
  //Serial.println();
  Wire.endTransmission();
  methaneReading = highReading * 256 + lowReading;
  logSingleData(String(methaneReading));
}

// Read time from RTC and update integer array variable "timeArray" in {year, month, day, hour, month, second} format
// Would've just made this function return the value like the rest of these "read" functions but C++ can't return arrays :(
// This whole function might not be necessary, might want to just call rtc.now() elsewhere
void readTime() {
  // get time and date from RTC and save in variables
  DateTime rtcTime = rtc.now();

  // Update global time array
  /*
  timeArray[0] = rtcTime.year();
  timeArray[1] = rtcTime.month();
  timeArray[2] = rtcTime.day();
  timeArray[3] = rtcTime.hour();
  timeArray[4] = rtcTime.minute();
  timeArray[5] = rtcTime.second();
  */
  logSingleData(String(rtcTime.year()));
  logSingleData(String(rtcTime.month()));
  logSingleData(String(rtcTime.day()));
  logSingleData(String(rtcTime.hour()));
  logSingleData(String(rtcTime.minute()));
  logSingleData(String(rtcTime.second()));
}



/***********************************************************************************/
/* ------------------------------ WRITING FUNCTIONS ------------------------------ */
/***********************************************************************************/


// Write a data entry to CSV file
// data array should take format: {year, month, day, hour, minute, second, methane reading, temp reading, salinity reading}
void logData(String data[9]) {
  // Not sure if this function should open and close the file each time, but it does for now.
  //File dataFile = SD.open(fileName, FILE_WRITE);
  dataFile = SD.open("mdata.csv", FILE_WRITE);

  for (int i = 0; i < 9; i++) {
    dataFile.print(data[i]);
    if (i < 8) dataFile.print(",");
  }
  dataFile.println();

  dataFile.close();
  
  Serial.println("New data logged!");
  Serial.println();
}

// Write a data entry to CSV file
// data array should take format: {year, month, day, hour, minute, second, methane reading, temp reading, salinity reading}
void logSingleData(String data) {
  // Not sure if this function should open and close the file each time, but it does for now.
  //File dataFile = SD.open(fileName, FILE_WRITE);
  dataFile = SD.open("mdata.csv", FILE_WRITE);

  dataFile.print(data);
  dataFile.print(",");

  dataFile.close();
  
  //Serial.println("New Single data logged!");
  //Serial.println();
}

// Write a data entry to CSV file
// data array should take format: {year, month, day, hour, minute, second, methane reading, temp reading, salinity reading}
void logDataNewLine() {
  // Not sure if this function should open and close the file each time, but it does for now.
  //File dataFile = SD.open(fileName, FILE_WRITE);
  dataFile = SD.open("mdata.csv", FILE_WRITE);

  dataFile.println();

  dataFile.close();
  
  Serial.println("End of data logged!");
  Serial.println();
}



// Write current sensor data to serial monitor
// data array should take format: {year, month, day, hour, minute, second, methane reading, temp reading, salinity reading}
void logToSerial(String data[9]) {

  // Date
  Serial.print(data[0]);
  Serial.print("/");
  Serial.print(data[1]);
  Serial.print("/");
  Serial.print(data[2]);

  Serial.print(" - ");

  // Time
  if (data[3].length() < 2) Serial.print("0"); // zero padding
  Serial.print(data[3]);
  Serial.print(":");
  if (data[4].length() < 2) Serial.print("0");
  Serial.print(data[4]);
  Serial.print(":");
  if (data[5].length() < 2) Serial.print("0");
  Serial.println(data[5]);

  // Methane
  Serial.print("Methane Concentration (%Vol * 100): ");
  Serial.println(data[6]);

  // Temperature
  Serial.print("Temperature (*C): ");
  Serial.println(data[7]);

  // Salinity
  Serial.print("Salinity (mg/L): ");
  Serial.println(data[8]);
}



/***************************************************************************************/
/* ------------------------------ CALIBRATION FUNCTIONS ------------------------------ */
/***************************************************************************************/


// Calibrate the zero point of the sensor (I'm assuming you just run this command in a zero methane environment)
// Planning to have this be triggered by serial command
void calibrateZero() {
  Wire.beginTransmission(byte(0x55));
  Wire.write(byte(0xA0));
  for (int i = 0; i < 8; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - 0xA0));
  Wire.endTransmission();
}

// Calibrate current value of methane
// Planning to have this be triggered by serial command
void calibrateSpan(unsigned int range) {
  // Split range into two bytes
  int highByte = range / 256;
  int lowByte = range % 256;

  int checksum = 0xAA + highByte + lowByte;

  // Send "calibrate span point" command
  Wire.beginTransmission(byte(0x55));
  Wire.write(byte(0xAA));

  Wire.write(byte(highByte));
  Wire.write(byte(lowByte));

  for (int i = 0; i < 6; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - (checksum % 256)));
  Wire.endTransmission();

}

// Recalibrate the clock module, type 'u' in Serial Monitor to activate
void updateRTC()
{
  
  Serial.println("Edit Mode...");

  // ask user to enter new date and time
  const char txt[6][15] = { "year [4-digit]", "month [1~12]", "day [1~31]",
                            "hours [0~23]", "minutes [0~59]", "seconds [0~59]"};
  String str = "";
  long newDate[6];

  while (Serial.available()) {
    Serial.read();  // clear serial buffer
  }

  for (int i = 0; i < 6; i++) {

    Serial.print("Enter ");
    Serial.print(txt[i]);
    Serial.print(": ");

    while (!Serial.available()) {
      ; // wait for user input
    }

    str = Serial.readString();  // read user input
    newDate[i] = str.toInt();   // convert user input to number and save to array

    Serial.println(newDate[i]); // show user input
  }

  // update RTC
  rtc.adjust(DateTime(newDate[0], newDate[1], newDate[2], newDate[3], newDate[4], newDate[5]));
  Serial.println("RTC Updated!");
}



/*****************************************************************************************/
/* ------------------------------ SUPPLEMENTARY FUNCTIONS ------------------------------ */
/*****************************************************************************************/


// Only a separate function to keep calculation info separated
float soilSalinityConversion(int soil_ec) {
//  TDS (mg/L or ppm) = EC (dS/m) x 800 (EC > 5 dS/m) 
//  1 dS/m, = 1000 µS/cm
//  TDS (mg/L or ppm) = EC (uS/cm)/1000 x 800 (EC > 5 dS/m) 
//  TDS (mg/L or ppm) = EC (uS/cm) x 0.8 (EC > 0.005 uS/cm) 
// https://ucanr.edu/sites/Salinity/Salinity_Management/Salinity_Basics/Salinity_measurement_and_unit_conversions/

  float soil_salinity = soil_ec * 0.8;  
  return soil_salinity;
}

void display_freeram() {

  Serial.print(F("- SRAM left: "));

  Serial.println(freeRam());

}


int freeRam() {

  extern int __heap_start,*__brkval;

  int v;

  return (int)&v - (__brkval == 0  ? (int)&__heap_start : (int) __brkval);  

}