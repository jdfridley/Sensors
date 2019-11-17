//Lora Transmitter for Fridley Lab EMUs, v1
//Unit GL1
//Data codes: A = air temp, H = RH, P = pressure, V = battery voltage

//Libraries
#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"
#define URTCLIB_MODEL_DS3231 2
  uRTCLib rtc(0x68);
#include <SPI.h>
#include <LoRa.h>
//#include <BH1750FVI.h>
//BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
#include <BME280I2C.h>
BME280I2C bme; 
#include <Adafruit_ADS1015.h>
  Adafruit_ADS1115 ads1115(0x48);

void setup() {
  Serial.begin(115200); //must connect non-power serial to TX/GND on Wemos board for 
  Serial.println("Serial OK");
  Wire.begin();

  //RTC 
  rtc.refresh();
  rtc.set_model(URTCLIB_MODEL_DS3231);
  rtc.set_rtc_address(0x68);
  //rtc.alarmSet(URTCLIB_ALARM_TYPE_1_FIXED_MS, 0, 0, 0, 1);
  rtc.alarmSet(URTCLIB_ALARM_TYPE_1_FIXED_S, 10, 0, 0, 1); //reset 10s past start of each minute
      //char y  = rtc.year();
      //char mo = rtc.month();
      //char d  = rtc.day();
      //char h  = rtc.hour();
      //char m  = rtc.minute();
  /*
    if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  */  
  // Only used once, then disabled
  //rtc.set(0, 42, 15, 7, 9, 11, 19);
  //  RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)


  /*  LightSensor.begin();  
    uint16_t lux = LightSensor.GetLightIntensity();
    float par;
    par = 22 + (0.019 * lux) - (1.90 * pow(10,-8) * lux * lux);
  */

  //Analog inputs (eg soil moisture)
  ads1115.begin();               // Initialize ads1115
  delay(10);
  ads1115.setGain(GAIN_ONE);     // GAIN ONE output to about 26,000 at max V with 3.3V
  int16_t out;
  float vmc;
  out = ads1115.readADC_SingleEnded(0);
  delay(10);
  vmc = 0.52 - (2.46 * pow(10,-5) * out) + (2.54 * pow(10,-10) * out * out);

  //BME temp-RH-pressure
  bme.begin();
  delay(10);
  float temp(NAN), hum(NAN), pres(NAN);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);
  delay(10);

  //battery voltage
  float power = (analogRead(A0)/1024.00) * 6.50; //this reads the 5V line through the resistor to A0
  delay(10);

  String label = String("GL1"); //Green Lakes unit 1
  String payload = label + "," + rtc.month() + "-" + rtc.day() + "-" + rtc.year() + "," + rtc.hour() + ":" + rtc.minute() + "," + "V" + power + "," + "A" + temp + "," + "H" + hum + "," + "P" + pres;
  Serial.print("String sent: ");
  Serial.println(payload);

  //Setup LoRa
  LoRa.setPins(15,2,0);
  delay(50);
  LoRa.begin(915E6);  //must be equal on receiver
  delay(50);
  LoRa.setSyncWord(0xF3); //must be equal on receiver
  LoRa.setTxPower(25); //max is 30; default is 17
  LoRa.setSpreadingFactor(10); //between 6 and 12; the higher the value, the longer the transmission time
  
  //Send LoRa data
  LoRa.beginPacket();
  delay(50);
  LoRa.print(payload);
  delay(500);
  LoRa.endPacket(true);
  delay(2000);

  rtc.alarmClearFlag(URTCLIB_ALARM_1); //clear alarm, turn off EMU for 1 min
  //rtc.alarmClearFlag(URTCLIB_ALARM_2);
}

void loop() {
}
