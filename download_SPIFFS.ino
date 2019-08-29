// based on SPIFFS_simple
// read data stored in SPIFFS memory
// Jordan Stark, July 2019

// This code will read the data stored in a SPIFFS file out to the serial monitor
// To save the data, use puTTY or a similar program


#include "FS.h" // this is the ESP8266 file system library

void setup() {
  Serial.begin(9600);
  SPIFFS.begin();

  delay(2000);

  File f = SPIFFS.open("/testdata.csv", "a");  //open in annotate mode to see how long it is - file name must match exactly
  if (!f) {
      Serial.println("file open failed");
  }  Serial.println("====== read length =======");
  
  

  f.println();
  float len = f.position(); //this determines the number of bytes in the file
  Serial.print(len);

  f.close();

  delay(2000);

  // open file for reading
  f = SPIFFS.open("/testdata.csv", "r"); //open file in read mode to print the data to the serial monitor
  if (!f) {
      Serial.println("file open failed");
  }  Serial.println("====== Reading from SPIFFS file =======");
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

void loop() {
}
