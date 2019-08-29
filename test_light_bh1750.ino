//test one or two BH1750 sensors
//based on example sketch from adafruit BH1750 library

#include <Wire.h>                   //I2C library
#include <BH1750.h>                 //light meter library
BH1750 lightMeter1(0x23);            // I2C address; default is ADDR->GND at 0x23
BH1750 lightMeter2(0x5c);                            // second option is ADDR->VCC at 0x5C
 /*
    BH1750 has six different measurement modes. They are divided in two groups;
    continuous and one-time measurements. In continuous mode, sensor continuously
    measures lightness value. In one-time mode the sensor makes only one
    measurement and then goes into Power Down mode.
    Each mode, has three different precisions:
      - Low Resolution Mode - (4 lx precision, 16ms measurement time)
      - High Resolution Mode - (1 lx precision, 120ms measurement time)
      - High Resolution Mode 2 - (0.5 lx precision, 120ms measurement time)
    By default, the library uses Continuous High Resolution Mode, but you can
    set any other mode, by passing it to BH1750.begin() or BH1750.configure()
    functions.
    [!] Remember, if you use One-Time mode, your sensor will go to Power Down
    mode each time, when it completes a measurement and you've read it.
    Full mode list:
      BH1750_CONTINUOUS_LOW_RES_MODE
      BH1750_CONTINUOUS_HIGH_RES_MODE (default)
      BH1750_CONTINUOUS_HIGH_RES_MODE_2
      BH1750_ONE_TIME_LOW_RES_MODE
      BH1750_ONE_TIME_HIGH_RES_MODE
      BH1750_ONE_TIME_HIGH_RES_MODE_2
  */
void setup() {
  Serial.begin(9600);
  Wire.begin();
  lightMeter1.begin(); //single meas, 1 lux precision
  


}
void loop() {
  float lux1 = lightMeter1.readLightLevel();
  lightMeter2.begin(); //single meas, 1 lux precision
 // float lux2 = lightMeter2.readLightLevel();
  
  Serial.print("Lux1: "); 
  Serial.println(lux1);  
//  Serial.print("Lux2: "); 
 // Serial.println(lux2); 
    
  delay(5000);

}
