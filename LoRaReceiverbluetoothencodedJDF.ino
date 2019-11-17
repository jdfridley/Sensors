#include <SPI.h>
#include <LoRa.h>
#include <SoftwareSerial.h>
  SoftwareSerial mySerial(7, 6);

void setup() {
  Serial.begin(57600);
  mySerial.begin(115200);
  delay(100);

  Serial.println("LoRa Receiver, FAYE Mesonet, Fridley Lab");

  LoRa.setPins(4,2,3);
  delay(50);
  LoRa.begin(915E6);            //must be same setting in transmitter
  LoRa.setSpreadingFactor(10);  //must be same setting in transmitter
  LoRa.setSyncWord(0xF3);     //must be same setting in transmitter
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    //Serial.print("Received packet '");

    // read packet
    String readout = "";
    while (LoRa.available()) {
      readout = readout + (char)LoRa.read(); //store characters as they are received
    }
    
    delay(500);
    //Bluetooth output
    Serial.print(readout);
    Serial.print(" RSSI="); //radio signal strength indicator
    Serial.print(LoRa.packetRssi());
    Serial.print(" SNR="); //signal-to-noise ratio
    Serial.print(LoRa.packetSnr());
    Serial.print(" packet.size="); //length of string received
    Serial.println(packetSize);

    delay(500);
    //Software Serial output
    mySerial.print(readout);
    delay(100);
    mySerial.print(" RSSI=");
    mySerial.print(LoRa.packetRssi());
    mySerial.print(" SNR=");
    mySerial.print(LoRa.packetSnr());
    mySerial.print(" packet.size=");
    mySerial.println(packetSize);
  }
}  
