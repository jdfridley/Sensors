
// Code to write data from sensors to SPIFFS memory in dataloggers
// For August 2019 trip to GSMNP


#include <Wire.h>

#include <Adafruit_ADS1015.h>     //Adafruit ADS1115 library-this controls the analog to digital converter      
  Adafruit_ADS1115 ads1115(0x48);     // sets I2C address for the ADC    

#include <BME280I2C.h>            //Arduino BME280 library to read temp, pressure, humidity over I2C
BME280I2C bme;

#include <BH1750.h>               //Arduino BH1750 library to read lux
BH1750 lightMeter1(0x23);             // sets I2C address for lux sensor -- if addr pin is connected to vcc, use 0x5c  

#include "FS.h"                   //File system library for SPIFFS

#include "uRTCLib.h"              //Library to run the RTC alarm -- there are other libraries available but this seemed most straightforward for just setting the alarm
#define URTCLIB_MODEL_DS3231 2        //RTC model
uRTCLib rtc(0x68);                    //RTC I2C address

#include <OneWire.h>              
#include <DallasTemperature.h>    //Both of these libraries are for the soil temp sensor
#define ONE_WIRE_BUS 2                // DS18B20 does not use I2C, runs on OneWire protocol
OneWire oneWire(ONE_WIRE_BUS);        //Initializes the OneWire protocol
DallasTemperature temps(&oneWire);

void setup() {
  Serial.begin(9600);
  SPIFFS.begin();
  Wire.begin();
  digitalWrite( SDA, LOW);      //This may not be necessary but could recover I2C if shut down incorrectly
  digitalWrite( SCL, LOW);

  //ds3231 clock
    rtc.set_model(URTCLIB_MODEL_DS3231);
    rtc.set_rtc_address(0x68);
    rtc.alarmSet(URTCLIB_ALARM_TYPE_1_FIXED_MS, 0, 0, 0, 1); // this sets the alarm to turn the system on and run code every hour on the hour
        // other alarm types are  URTCLIB_ALARM_TYPE_1_ALL_S - Every second
                                //URTCLIB_ALARM_TYPE_1_FIXED_S - Every minute at given second
                                //URTCLIB_ALARM_TYPE_1_FIXED_MS - Every hour at given Minute:Second
                                //URTCLIB_ALARM_TYPE_1_FIXED_HMS - Every day at given Hour:Minute:Second
                                //URTCLIB_ALARM_TYPE_1_FIXED_DHMS - Every month at given DAY-Hour:Minute:Second
                                //URTCLIB_ALARM_TYPE_1_FIXED_WHMS - Every week at given DOW + Hour:Minute:Second
                                //URTCLIB_ALARM_TYPE_2_ALL_M - Every minute at 00 Seconds
                                //URTCLIB_ALARM_TYPE_2_FIXED_M - Every hour at given Minute(:00)
                                //URTCLIB_ALARM_TYPE_2_FIXED_HM - Every day at given Hour:Minute(:00)
                                //URTCLIB_ALARM_TYPE_2_FIXED_DHM - Every month at given DAY-Hour:Minute(:00)
                                //URTCLIB_ALARM_TYPE_2_FIXED_WHM - Every week at given DOW + Hour:Minute(:00)
        // for _FIXED_ alarms, the integers set the time of the alarm -- second, minute, hour, dayofweek
            //so for an alarm every hour at :30, (URTCLIB_ALARM_TYPE_1_FIXED_MS, 0, 30, 0, 1)
        // the library uses 1 as default for dayofweek so I haven't changed that but it doesn't affect most alarms
        // should be possible to alternate alarms 1 and 2 for more intervals but I haven't gotten that working

  //ads1115 analog to digital converter for vmc
    ads1115.begin();    
    ads1115.setGain(GAIN_ONE); //set gain on the ADC

  //bme 280
    bme.begin(); 
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius); // set units on the BME to C and Pa (humidity is %)
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  //bh1750
    lightMeter1.begin(); 
 
  //OneWire for soil temp probe
    temps.begin();

  //Initialize SPIFFS
  File f = SPIFFS.open("/data.csv", "a"); // open SPIFFS file in annotate mode to add one line of data at the end

  
  // collect sensor data
      rtc.refresh(); //update time from the rtc
   
      int y  = rtc.year();
      int mo = rtc.month();
      int d  = rtc.day();
      int h  = rtc.hour();
      int m  = rtc.minute();
      
      int raw_vmc_2 = ads1115.readADC_SingleEnded(2); 
      int raw_vmc_3 = ads1115.readADC_SingleEnded(3); //read the vmc sensors through the ADC
      
      lightMeter1.readLightLevel(); //read the lux
      
      temps.requestTemperatures();
      temps.getTempCByIndex(0); //read the soil temperature
      
      delay(800); // lux and soil temp both require delay after first read before reading accurately
                  // 500 is enough for lux but not soil temp; 750 gives some ok readings but not all

      // these readings are what are actually written to file
      float lux1 = lightMeter1.readLightLevel();  // lux
      float soiltemp = temps.getTempCByIndex(0);  // soil temp
      
      float pres;   // BME
      float temp;
      float hum;
      bme.read(pres, temp, hum, tempUnit, presUnit);

      int power = analogRead(A0); //this reads the 5V line through the resistor to A0

  // write sensor data to file
      f.print(y);
      f.print(",");
      f.print(mo);
      f.print(",");
      f.print(d);
      f.print(",");
      f.print(h);
      f.print(",");
      f.print(m);
      f.print(",");
      f.print(raw_vmc_2);
      f.print(",");
      f.print(raw_vmc_3);
      f.print(",");
      f.print(lux1);
      f.print(",");
      f.print(soiltemp);
      f.print(",");
      f.print(pres);
      f.print(",");
      f.print(temp);
      f.print(",");
      f.print(hum);
      f.print(",");
      f.print(power);
      f.print(",");
      f.print("a30");     // EDIT FOR EACH SENSOR
      f.println();
  
  // close file
      f.close();        // Data is not actually written to file until this runs

  // reset clock flag to turn power off
      rtc.alarmClearFlag(URTCLIB_ALARM_1);

  // if the alarm doesn't work to turn off    
  
  f = SPIFFS.open("/data.csv", "a");
  f.println("err1");
  f.close();                                    // Err1 = ESP did not turn off after clearing the alarm flag. 
                                                    //This will appear once in data when ESP is plugged in because it cannot power off

  delay(60*1000);
  //ESP.deepSleep(60*1000000,WAKE_RF_DISABLED); //this can be used instead of ESP.restart if D0 is connected to rst
                                                //but that makes it so that new code cannot be uploaded to the ESP
  ESP.restart();

  
  f = SPIFFS.open("/data.csv", "a");  // errors after this should never appear unless there is something really wrong
  f.println("err2");                  // different numbers to allow diagnosis of where the ESP got stuck
  f.close();
}


void loop() {
  // if microcontroller gets stuck in loop -- should never be run

  File f = SPIFFS.open("/data.csv", "a");
  f.println("err3");
  f.close();
  
  ESP.restart();
  delay(60*1000);

  f = SPIFFS.open("/data.csv","a");
  f.println("err4");
  f.close();
}
