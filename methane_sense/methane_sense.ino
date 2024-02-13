/* --------------------------------------
  methane_sense

  Read data from MH-741A Methane Sensor via I2C, and write to a CSV file on an SD card

  If currently calibrated, concentration value ranges from 0 to 10000 with 10000 being 100% by volume

  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
   MH-741A Methane Sensor wires connected as follows:
 ** RED - 5V
 ** BLACK - GND
 ** YELLOW - SCL
 ** BROWN - SDA

   --------------------------------------
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>

File dataFile; // data file to write methane data to
String fileName = "mdata.csv"; // NAME OF FILE (before .csv extension) MUST BE 8 CHARACTERS OR FEWER
int timeDelay = 5000; // time between data entries in milliseconds

int reading = 0; // to be received from sensor
int highDensityReading = 0;
int lowDensityReading = 0;
int lowDensityRange = 0;
int highDensityRange = 0;

int address = 0x55; // Address and commands are from datasheet
int gasConcCommand = 0x96; // Gas concentration
int calibrZeroCommand = 0xA0; // Calibrate zero point
int calibrSpanCommand = 0xAA; // Calibrate span point

String concentrationList = "";

unsigned long startTime;

int iter = 0;


void setup() {
  Wire.begin();

  Serial.begin(9600);
  digitalWrite(SDA, LOW); // DISABLE PULL UP RESISTORS
  digitalWrite(SCL, LOW);

  initSD(); // Comment this out to only use methane sensor, along with line 63

  //calibrateZero();
  //calibrateSpan(1500); // KEEP COMMENTED OUT IF NOT CALIBRATING

  startTime = millis();
}

void loop() {
  senseMethane();

  logToSerial();
  logData(); // Comment this out to only use methane sensor, along with line 54

  delay(timeDelay);
}

// Read the methane value from the sensor, returns computed methane value, also saves relevant raw data in global variables
int senseMethane() {
  Wire.beginTransmission(byte(address));
  Wire.write(byte(gasConcCommand));
  for (int i = 0; i < 8; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - gasConcCommand));
  Wire.endTransmission();

  delay(70); // Not sure if this is needed

  Wire.requestFrom(address, 10); // Request ten bytes

  int i = 0;
  while (Wire.available()) {
    reading = Wire.read(); 
    if (i == 5) {
      highDensityReading = reading;
    } else if (i == 6) {
      lowDensityReading = reading;
    } else if (i == 7) {
      highDensityRange = reading;
    } else if (i == 8) {
      lowDensityRange = reading;
    }
    //reading = reading << 8; // not sure what these lines do; shift binary digits 8 spaces to the left (surely resulting in 00000000?),
    //reading |= Wire.read(); // combining (bitwise OR) what remains to another Wire.read()? 
    //Serial.println(reading);
    i++;
  }
  return highDensityReading * 256 + lowDensityReading;
}

// Calibrate the zero point of the sensor (I'm assuming you just run this command in a zero methane environment)
void calibrateZero() {
  Wire.beginTransmission(byte(address));
  Wire.write(byte(calibrZeroCommand));
  for (int i = 0; i < 8; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - calibrZeroCommand));
}

// Calibrate the range of output in ppm(?)
void calibrateSpan(unsigned int range) {
  // Split range into two bytes
  int highByte = range / 256;
  int lowByte = range % 256;

  // TESTING
  Serial.print("High Span: ");
  Serial.println(highByte);
  Serial.print("Low Span: ");
  Serial.println(lowByte);

  int checksum = calibrSpanCommand + highByte + lowByte;

  // Send "calibrate span point" command
  Wire.beginTransmission(byte(address));
  Wire.write(byte(calibrSpanCommand));

  Wire.write(byte(highByte));
  Wire.write(byte(lowByte));

  for (int i = 0; i < 6; i++) {
    Wire.write(byte(0x00));
  }
  Wire.write(byte(256 - (checksum % 256)));
  Wire.endTransmission();

}

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

// Write a data entry to CSV file
void logData() {
  // Not sure if this function should open and close the file each time, but it does for now.
  dataFile = SD.open(fileName, FILE_WRITE);

  dataFile.print((millis() - startTime) / 1000); // Log the time in seconds
  dataFile.print(",");
  dataFile.println(256 * highDensityReading + lowDensityReading); // Log value of methane given by sensor

  dataFile.close();
  
  Serial.println("New data logged!");
  Serial.println();
}

// Write current methane data to serial monitor
void logToSerial() {
  char time[15];
  sprintf(time, "Time: %.02d:%.02d", int((millis() - startTime) / 60000), int((millis() - startTime) / 1000 % 60));
  
  Serial.println(time);

  Serial.print("Methane Concentration: ");
  Serial.println(256 * highDensityReading + lowDensityReading);
  Serial.print("Range Value: ");
  Serial.println(256 * highDensityRange + lowDensityRange);

  //concentrationList = concentrationList.equals("") ? String(256 * highDensityReading + lowDensityReading) : String(concentrationList + ", " + (256 * highDensityReading + lowDensityReading));
  //Serial.print("[");
  //Serial.print(concentrationList);
  //Serial.println("]");
  //Serial.print("Gas Concentration Range: ");
  //Serial.println(256 * highDensityRange + lowDensityRange);

  Serial.println();
}
