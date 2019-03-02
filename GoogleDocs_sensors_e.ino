/*  HTTPS on ESP8266 with follow redirects, chunked encoding support
 *  Version 3.0
 *  Author: Sujay Phadke
 *  Github: @electronicsguy
 *  Copyright (C) 2018 Sujay Phadke <electronicsguy123@gmail.com>
 *  All rights reserved.
 *
 *  Example Arduino program
 */

#include <Wire.h>                   //I2C library; on WEMOS D2 R1 chip, SCL is D1 pin and SDA is D2 pin
#include <Adafruit_ADS1015.h>       //ADC library for ADS1115
#include <BH1750.h>                 //light meter library
#include <Adafruit_SHT31.h>         //Sensiron SHT31 temp and humidity sensor

Adafruit_SHT31 sht31 = Adafruit_SHT31();     //default address is 0x44; changeable to 0x45 with ADR pin to VCC
Adafruit_ADS1115 ads1115(0x48);     // default 0x48 address (wiring of ADDR to GND): moisture
BH1750 lightMeter(0x23);            // I2C address; default is ADDR->GND at 0x23
                                    // second option is ADDR->VCC at 0x5C
#include "RTClib.h"
RTC_DS3231 rtc;
#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"

// Fill ssid and password with your network credentials
const char* ssid = "ZB934";
const char* password = "SM6PHQSRTBSKPPJL";

const char* host = "script.google.com";
// Replace with your own script id to make server side changes
const char *GScriptId = "AKfycbzlMx0uSj5A16r02f6J4Ee_I-oOGit7JrdIQnVHntX8KYfpL-bR";

const int httpsPort = 443;

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "";
//const uint8_t fingerprint[20] = {};

// Write to Google Spreadsheet
String url = String("/macros/s/") + GScriptId + "/exec?value=Hello";
// Fetch Google Calendar events for 1 week ahead
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";
// Read from Google Spreadsheet
String url3 = String("/macros/s/") + GScriptId + "/exec?read";

String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"Sheet1\", \
                    \"values\": ";
String payload = "";

HTTPSRedirect* client = nullptr;
// used to store the values of free stack and heap
// before the HTTPSRedirect object is instantiated
// so that they can be written to Google sheets
// upon instantiation

void setup() {
  Serial.begin(115200);
  Serial.flush();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  ads1115.begin();               // Initialize ads1115
  ads1115.setGain(GAIN_ONE);     // GAIN ONE output to about 26,000 at max V with 3.3V
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE); //single measurement, 1 lux precision
  sht31.begin(0x44);             // address of SHT31
  
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  // flush() is needed to print the above (connecting...) message reliably, 
  // in case the wireless connection doesn't go through
  Serial.flush();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i=0; i<5; i++){
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
       flag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }

  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
}

void loop() {

  DateTime now = rtc.now();
  String month = String(now.month(), DEC);
  String datetime = month + "/" + now.day() + "/" + now.year() + " " + now.hour() + ":" + now.minute() + ":" + now.second();

  int16_t out;
  float vmc;
  out = ads1115.readADC_SingleEnded(0);
  delay(10);

  //convert the value to VWC
  //conversion from Mickley paper calibrations on github:
  //https://github.com/mickley/EMU/blob/master/EMU-Analysis/Analyses/Calibration/Calibration.md
  vmc = 0.52 - (2.46 * pow(10,-5) * out) + (2.54 * pow(10,-10) * out * out);

  //read from the SHT31
  float t = sht31.readTemperature(); //C temp from SHT31 sensor
  float h = sht31.readHumidity(); //humidity from SHT31 sensor

  //read lux
  float lux = lightMeter.readLightLevel();
  float par;
  //convert the value to PAR
  //conversion from Mickley paper calibrations on github:
  //https://github.com/mickley/EMU/blob/master/EMU-Analysis/Analyses/Calibration/Calibration.md
  par = 22 + (0.019 * lux) - (1.90 * pow(10,-8) * lux * lux);

  //read voltage from A0 connected to 330 resistor (which is connected to battery V+)
  double volts = (analogRead(A0)/1024.00) * 6.50; //assumes 330K resistor
  
  static int error_count = 0;
  static int connect_count = 0;
  const unsigned int MAX_CONNECT = 20;
  static bool flag = false;
  
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }

  if (client != nullptr){
    if (!client->connected()){
      client->connect(host, httpsPort);
      payload = payload_base + "\"" + datetime + "," + t + "," + h + "," + vmc + "," + par + "," + volts + "\"}";
      client->POST(url2, host, payload, false);
    }
  }
  else{
    DPRINTLN("Error creating client object!");
    error_count = 5;
  }
  
  if (connect_count > MAX_CONNECT){
    //error_count = 5;
    connect_count = 0;
    flag = false;
    delete client;
    return;
  }

  Serial.println("POST append memory data to spreadsheet:");
  payload = payload_base + "\"" + datetime + "," + t + "," + h + "," + vmc + "," + par + "," + volts + "\"}";
  if(client->POST(url2, host, payload)){
    ;
  }
  else{
    ++error_count;
    DPRINT("Error-count while connecting: ");
    DPRINTLN(error_count);
  }
  Serial.println(payload);
  
  if (error_count > 3){
    Serial.println("Halting processor..."); 
    delete client;
    client = nullptr;
    Serial.flush();
    ESP.deepSleep(0);
  }
  
  // In my testing on a ESP-01, a delay of less than 1500 resulted 
  // in a crash and reboot after about 50 loop runs.
  delay(30000);
                          
}
