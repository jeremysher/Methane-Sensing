
#include <Wire.h>                   // for I2C communication
#include <RTClib.h>                 // for RTC
#include <Arduino.h>
#include <Adafruit_SHT31.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31(); //temperature sensor

RTC_DS3231 rtc;                     // create rtc for the DS3231 RTC module, address is fixed at 0x68

void setup() { 
  //Intialize I2C peripherals
  Wire.setClock(400000);
  Wire.begin();

   //Intialize Serial Monitor
  Serial.begin(9600);

  
  //Intialize real time clock module peripheral
  rtc.begin();
  //delay(10000);
  Serial.println("RTC Intialized");

  //Intialize Temperature Sensor
  //sht31.begin(0x44);
  //Serial.println("SHT31 Intialized");
}

void loop() {
  rtc.begin();
    //Print Time
  displayTime();
  //Delay One Second 
  //Print Temperature
  /*float t = sht31.readTemperature();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  } */


  //Check if edit update is necessary for time
  /*
  if (Serial.available()) {
  char input = Serial.read();
  if (input == 'u') updateRTC();  // update RTC time
  }
  Serial.println(" ");
*/
  delay(3000);

}
/*
   function to display time to Serial text
*/
void displayTime()
{

  /*
     create array to convert digit days to words:

     0 = Sunday    |   4 = Thursday
     1 = Monday    |   5 = Friday
     2 = Tuesday   |   6 = Saturday
     3 = Wednesday |
  */
  const char dayInWords[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

  /*
     create array to convert digit months to words:

     0 = [no use]  |
     1 = January   |   6 = June
     2 = February  |   7 = July
     3 = March     |   8 = August
     4 = April     |   9 = September
     5 = May       |   10 = October
     6 = June      |   11 = November
     7 = July      |   12 = December
  */
  const char monthInWords[13][4] = {" ", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  // get time and date from RTC and save in variables
  DateTime rtcTime = rtc.now();

  int ss = rtcTime.second();
  int mm = rtcTime.minute();
  int hh = rtcTime.twelveHour();
  int DD = rtcTime.dayOfTheWeek();
  int dd = rtcTime.day();
  int MM = rtcTime.month();
  int yyyy = rtcTime.year();

 

  // print date in dd-MMM-yyyy format and day of week
  if (dd < 10) Serial.print("0");  // add preceeding '0' if number is less than 10
  Serial.print(dd);
  Serial.print("-");
  Serial.print(monthInWords[MM]);
  Serial.print("-");
  Serial.print(yyyy);

  Serial.print("  ");
  Serial.println(dayInWords[DD]);


  // print time in 12H format
  if (hh < 10) Serial.print("0");
  Serial.print(hh);
  Serial.print(':');

  if (mm < 10) Serial.print("0");
  Serial.print(mm);
  Serial.print(':');

  if (ss < 10) Serial.print("0");
  Serial.print(ss);

  if (rtcTime.isPM()) Serial.println(" PM"); // print AM/PM indication
  else Serial.println(" AM");
}

/*
   function to update RTC time using user input
*/
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