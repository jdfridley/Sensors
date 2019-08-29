// test all sensors used for prototype EMU

#include <Wire.h>
#include <Adafruit_ADS1015.h>           //ADC code
Adafruit_ADS1115 ads1115(0x48);         //one of 4 addresses

#include <BME280I2C.h>
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

#include <BH1750.h>                 //light meter library
BH1750 lightMeter1(0x23);            // I2C address; default is ADDR->GND at 0x23


#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temps(&oneWire);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  digitalWrite( SDA, LOW);
  digitalWrite( SCL, LOW);

  //ads1115
    ads1115.begin();             // Initialize ads1115
    ads1115.setGain(GAIN_ONE);   // GAIN ONE output to about 26,000 at max V with 3.3V

  //BME280
    bme.begin();


  //bh1750 x2
    lightMeter1.begin(); 
   // lightMeter2.begin(); 

  //onewire soil temp
    temps.begin();
}

void loop() {
  // collect sensor data
   
   
   rtc.refresh();
   
      int y  = rtc.year();
      int mo = rtc.month();
      int d  = rtc.day();
      int h  = rtc.hour();
      int m  = rtc.minute();
      
      int raw_vmc_2 = ads1115.readADC_SingleEnded(2);
      int raw_vmc_3 = ads1115.readADC_SingleEnded(3);
      
      BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
      BME280::PresUnit presUnit(BME280::PresUnit_Pa);
      
      lightMeter1.readLightLevel();
      //lightMeter2.readLightLevel();
      temps.requestTemperatures();
      temps.getTempCByIndex(0);
      delay(800); // lux and soil temp both require delay after first read before reading accurately
                  // 500 is enough for lux but not soil temp; 750 gives some ok readings but not all
      
      float lux1 = lightMeter1.readLightLevel();
      //float lux2 = lightMeter2.readLightLevel();
      float soiltemp = temps.getTempCByIndex(0);
      
      float pres;
      float temp;
      float hum;
      bme.read(pres, temp, hum, tempUnit, presUnit);

      int power = analogRead(A0);

      float tempdiff = soiltemp - temp;
      float vmcdiff = raw_vmc_2 - raw_vmc_3;
      
  Serial.print("Time:");
  Serial.print(y);
  Serial.print(",");
  Serial.print(mo);
  Serial.print(",");
  Serial.print(d);
  Serial.print("  ");
  Serial.print(h);
  Serial.print(":");
  Serial.print(m);
  Serial.println();


 // test whether sensor readings are reasonable 
 // this is arbitrary based on what I have seen in the lab
 // also prints the actual sensor values which may be more useful!
  
  if( lux1 > 0 && lux1<65000){
    Serial.println("lux 1 good");
  } 
  else{
    Serial.println("check lux 1"); 
  }


  
  if(raw_vmc_2>26000 && raw_vmc_2<30000){
    Serial.println("vmc2 good");
  }
  else{
    Serial.println("check vmc2"); 
  }
  if(raw_vmc_3>26000 && raw_vmc_3<30000){
    Serial.println("vmc3 good");
  }
  else{
    Serial.println("check vmc3"); 
  }

  if(abs(vmcdiff) < 5){
    Serial.println("vmc match");
  }
  else{
    Serial.println("no vmc match"); 
  }
  
  if(soiltemp>20 && soiltemp <30){
    Serial.println("soiltemp good");
  }
  else{
    Serial.println("check soil temp"); 
  }
  if(temp>20 && temp <30){
    Serial.println("BME temp good");
  }
  else{
    Serial.println("check BME"); 
  }
  if(abs(tempdiff) < 1){
    Serial.println("temps match");
  }
  else{
    Serial.println("no temp match"); 
  }

  
  Serial.print(raw_vmc_2);
  Serial.print(",");
  Serial.print(raw_vmc_3);
  Serial.print(",");
  Serial.print(lux1);
  Serial.print(",");
  Serial.print(soiltemp);
  Serial.print(",");
  Serial.print(pres);
  Serial.print(",");
  Serial.print(temp);
  Serial.print(",");
  Serial.print(hum);
  Serial.print(",");
  Serial.print(power);
  Serial.println();

  delay(10000);


}
