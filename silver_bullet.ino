// Fridley Lab Silver Bullet Sensor: Arduino code to launch, store, and write data from sensors to SPIFFS memory in dataloggers
// https://www.fridleylab.org/protocols
// Original script by Jordan Stark (2019)
// Modifications 2024, Jason Fridley, Jeff Hamilton


#include <Wire.h>

#include <BME280I2C.h>            //Arduino BME280 library to read temp, pressure, humidity over I2C
BME280I2C bme;
BME280::TempUnit tempUnit(BME280::TempUnit_Celsius); 
BME280::PresUnit presUnit(BME280::PresUnit_Pa);

// The I2C address is 0x23 (ADDR pin is low) or 0x5C (ADDR pin is high).
#define BH1750_I2C_ADDRESS_1        0x23
#define BH1750_I2C_ADDRESS_2        0x5C
#define BH1750_POWER_DOWN           0x00    // Stop all operations
#define BH1750_POWER_ON             0x01    // Power on and wait for measurement command
#define BH1750_RESET                0x07    // Clears the illuminance data register, does not work in power down.
#define BH1750_CONT_H_RES_MODE      0x10
#define BH1750_CONT_H_RES_MODE2     0x11
#define BH1750_CONT_L_RES_MODE      0x13
#define BH1750_ONE_TIME_H_RES_MODE  0x20
#define BH1750_ONE_TIME_H_RES_MODE2 0x21
#define BH1750_ONE_TIME_L_RES_MODE  0x23
#define BH1750_MEAS_TIME_H          0x40
#define BH1750_MEAS_TIME_L          0x60

const float BH1750_factor = 1.2;
const int _i2c_address = BH1750_I2C_ADDRESS_1;


#include "FS.h"                   //File system library for SPIFFS

#include "uRTCLib.h"              //Library to run the RTC alarm -- there are other libraries available but this seemed most straightforward for just setting the alarm
#define URTCLIB_MODEL_DS3231 2        //RTC model
uRTCLib rtc(0x68);                    //RTC I2C address


void setup() {
  Serial.begin(9600);
  SPIFFS.begin();
  Wire.begin();

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


    bme.begin(); 

  //Initialize SPIFFS
  File f = SPIFFS.open("/data.csv", "a"); // open SPIFFS file in annotate mode to add one line of data at the end

  
  // collect sensor data
      rtc.refresh(); //update time from the rtc
   
      int y  = rtc.year();
      int mo = rtc.month();
      int d  = rtc.day();
      int h  = rtc.hour();
      int m  = rtc.minute();
      
      
      delay(800); // lux and soil temp both require delay after first read before reading accurately
                  // 500 is enough for lux but not soil temp; 750 gives some ok readings but not all

      // these readings are what are actually written to file

      int power = analogRead(A0); //this reads the 5V line through the resistor to A0

  //light readings
  write8( BH1750_POWER_ON);
  write8( mt_H( 31));          // lowest value of measurement time
  write8( mt_L( 31));
  write8( BH1750_ONE_TIME_H_RES_MODE);
  delay( 81);            // 180 * 31 / 69
  uint16_t c = read16();
  float lux_light = (float) c / BH1750_factor * ( 69.0 / 31.0 );       // BH1750_factor is usually 1.2
  float par;
  par = 22 + (0.019 * lux_light) - (1.90 * pow(10,-8) * lux_light * lux_light);
  
  //BME
  float pres;
  float temp;
  float hum;
  bme.read(pres, temp, hum, tempUnit, presUnit);

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
      f.print(temp);
      f.print(",");
      f.print(hum);
      f.print(",");
      f.print(pres);
      f.print(",");
      f.print(lux_light);
      f.print(",");
      f.print(par);
      f.print(",");
      f.print(power);
      f.print(",");
      f.print("EEE");     // EDIT FOR EACH SENSOR
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
  
  if(!Serial) {
    ESP.deepSleep(60e6); //this can be used instead of ESP.restart if D0 is connected to rst
    ESP.restart();
  }

}


void loop() {

  File f = SPIFFS.open("/data.csv", "a");
  f.println("loop1");
  f.close();

  // download data if connected to computer

  if(Serial) {
    File f = SPIFFS.open("/data.csv", "a");  //open in annotate mode to see how long it is - file name must match exactly
    if (!f) {
        Serial.println("file open failed");
      }  Serial.println("====== read length =======");
  
  

    f.println("loop2");
    float len = f.position(); //this determines the number of bytes in the file
    Serial.print(len);
  
    f.close();
  
    delay(2000);
  
    // open file for reading
    f = SPIFFS.open("/data.csv", "r"); //open file in read mode to print the data to the serial monitor
    if (!f) {
          Serial.println("file open failed");
      }  
    
    Serial.println("====== Reading from SPIFFS file =======");
    // read file
  
    for (int i=1; i<=len; i++){ 
      //this currently will keep printing a huge number of lines to the serial output
      //if you know how many bytes each line will be, change line above to read ... ; i<=len/[bytesperline];i++ ...
      //alternatively, just close the puTTY connection after it has stopped reading in data
      String s=f.readStringUntil('\n'); //reads the first line of the file
      Serial.print(i);
      Serial.print(":"); // if line numbers are not needed, comment out this line and the line above 
      Serial.println(s);
    }
  }
  
  ESP.restart();

}


//Below is for the BH1750 readings

int BH1750_begin()
{
  // Send command 'RESET' for extra safety. 
  // A 'RESET' is only valid with power on.
  // A 'RESET' only resets the illuminace data register.
  //
  // Three checks if the communication with the sensor is okay.
  // No more error checks after this.

  int error = 0;
  
  error = write8( BH1750_POWER_ON);
  if( error != 0)
    return( error);
    
  error = write8( BH1750_RESET);
  if( error != 0)
    return( error);

  error = write8( BH1750_POWER_DOWN);
  if( error != 0)
    return( error);

  return( error);  
}


inline uint8_t mt_H( uint8_t mt)
{
  return( BH1750_MEAS_TIME_H | (mt >> 5));  // highest 3 bits
}

inline uint8_t mt_L( uint8_t mt)
{
  return( BH1750_MEAS_TIME_L | (mt & 0x1F));   // lowest 5 bits
}


// Read the two bytes from the BH1750
// If an error occurs, then zero is returned.
uint16_t read16()
{
  uint16_t x;
  int n = Wire.requestFrom( _i2c_address, 2);
  if( n == 2)
  {
    byte highbyte = Wire.read();              // highest byte first
    byte lowbyte  = Wire.read();
    x = word( highbyte, lowbyte);
  }
  else
  {
    x = 0;
    Serial.println("ERROR I2C fail 1");          // for debugging
  }
  return( x);
}


// Write the command byte to the BH1750
// An error is returned, it is zero for no error.
int write8( uint8_t command)
{
  Wire.beginTransmission( _i2c_address);
  Wire.write( command);
  int error = Wire.endTransmission();

  if( error != 0)                               // for debugging
    Serial.println("ERROR I2C fail 2");         // for debugging
  
  return( error);
}
