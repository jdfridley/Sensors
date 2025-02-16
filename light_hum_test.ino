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


// Moving average array.
// A "moving average" is also called "rolling average" or "running average".
#define NUM_SAMPLES_AVG   10
float moving_avg[NUM_SAMPLES_AVG];
int index_avg;


const int _i2c_address = BH1750_I2C_ADDRESS_1;


// The default correction factor to calculate the lux is 1.2. 
// Adjust it when you are able to calibrate the BH1750.
// According to the datasheet, the value can be 0.96 to 1.44.
// It is not know if this correction factor is the same for the whole range.
const float BH1750_factor = 1.2;


void setup() 
{
  Serial.begin( 9600);
  Serial.println(F( "\nBH1750 lux sensor"));

  Wire.begin();
    //bme 280
    bme.begin(); 
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius); // set units on the BME to C and Pa (humidity is %)
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  // Start the sensor and test if the I2C address is right.
  int error = BH1750_begin();
  if( error != 0)
    Serial.println(F( "Error, BH1750 not found"));

  // Initialize the moving average array.
  for( int i=0; i<NUM_SAMPLES_AVG; i++)
  {
    moving_avg[i] = 0.0;                        // or fill it with the actual current lux value.
  }
  index_avg = 0;
}


void loop() 
{
  Serial.println(F( "------------------"));
  // ----------------------------------------
  // High resultion with minimum measurement time
  // ----------------------------------------
  write8( BH1750_POWER_ON);
  write8( mt_H( 31));          // lowest value of measurement time
  write8( mt_L( 31));
  write8( BH1750_ONE_TIME_H_RES_MODE);
  delay( 81);            // 180 * 31 / 69
  uint16_t c = read16();
  float lux_light = (float) c / BH1750_factor * ( 69.0 / 31.0 );       // BH1750_factor is usually 1.2
  float par;
  par = 22 + (0.019 * lux_light) - (1.90 * pow(10,-8) * lux_light * lux_light);
  float pres;   // BME
  float temp;
  float hum;
  bme.read(pres, temp, hum, tempUnit, presUnit);
  Serial.print(F( "100k range: "));
  Serial.println( lux_light);
  Serial.print("PAR: "); 
  Serial.println(par);
  Serial.print("Temp: "); 
  Serial.println(temp);
  Serial.print("Humidity %: "); 
  Serial.println(hum); 
  Serial.print("Pres: ");
  Serial.println(pres);
    // ----------------------------------------
  
  delay( 1000);
}


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
