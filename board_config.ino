// Run this code once to set the clock and configure the SPIFFS
// Before starting, go to Tools -> Flash Size and select "4M (3M SPIFFS)"
// Open com before starting to be able to see when configuration is complete

#include <Wire.h>
#include "RTClib.h" // this library is used to set the clock; the code here is based on its example sketch
#include "FS.h"

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup () {
  Serial.begin(9600);

  delay(3000); // wait for console opening

// FOR RTC
  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //this line sets the time--comment out if not needed 


    DateTime now = rtc.now(); //sets time in the ESP8266 to the time on the RTC
    
    Serial.print(now.year(), DEC); //prints the time -- this is useful even if not setting the time to check that RTC is functioning properly
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    delay(3000);

//FOR SPIFFS (WILL DELETE ALL DATA!) 
 Serial.println("Please wait 30 secs for SPIFFS to be formatted");
 SPIFFS.format(); // this formats the flash memory on the ESP and must be run before using SPIFFS. Comment out if there is stored data that shouldn't be deleted
 Serial.println("Spiffs formatted"); // make sure to wait for this to appear in serial monitor! It does take ~30 seconds

}

void loop () {
    
}
