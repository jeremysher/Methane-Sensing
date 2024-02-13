
/*
  Author: Michelle Frolio
  Description: This code if for testing the EC and Salinity Sensor
*/

#include <SoftwareSerial.h>
#include <Wire.h>
 
#define RE 6 //was 8
#define DE 2 //was 7
#define RO 5 //was 7
#define DI 3 //was 3
 
const byte ec[] = {0x01, 0x03, 0x00, 0x15, 0x00, 0x01, 0x95, 0xCE};
const byte salinity[] = {0x01, 0x03, 0x00, 0x14, 0x00, 0x01, 0xC4, 0x0E};
byte values[8];
 
SoftwareSerial mod(RO, DI); // was 2,3
 
 
void setup()
{
  Serial.begin(9600);
  mod.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
 
}
 
void loop()
{
/**************Soil EC Reading*******************/  
  int soil_ec = soil_EC_Reading ();
  delay(1000);
 
  Serial.print("Soil EC: ");
  Serial.print(soil_ec);
  Serial.println(" us/cm");

  float soil_salinity = soil_Salinity_Conversion(soil_ec);

  Serial.print("Soil Salinity: ");
  Serial.print(soil_salinity, 3);
  //Serial.print(soil_salinity);
  Serial.println(" mg/L");
   
  delay(2000);
}

/*
 * Function:  modeActivation 
 * Description: calls the appropriate functions for the different mode or parts of the project
 * Inputs:
 * Returns:
 */
int soil_EC_Reading (){
  int soil_ec;
  
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
 
  if (mod.write(ec, sizeof(ec)) == 8)
  {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 7; i++)
    {
      values[i] = mod.read();
      Serial.print(values[i], HEX);
    }
    Serial.println();
  }
  return soil_ec = int(values[3]<<8|values[4]);
}

float soil_Salinity_Conversion(int soil_ec){  
//  TDS (mg/L or ppm) = EC (dS/m) x 800 (EC > 5 dS/m) 
//  1 dS/m, = 1000 µS/cm
//  TDS (mg/L or ppm) = EC (uS/cm)/1000 x 800 (EC > 5 dS/m) 
//  TDS (mg/L or ppm) = EC (uS/cm) x 0.8 (EC > 0.005 uS/cm) 
// https://ucanr.edu/sites/Salinity/Salinity_Management/Salinity_Basics/Salinity_measurement_and_unit_conversions/

  float soil_salinity = soil_ec * 0.8;  
  return soil_salinity;
}
