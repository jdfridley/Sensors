// Test sketch for clocks and MOSFETS
// This code with make the internal LED on the d1 Mini blink 3x
// If plugged into computer or if the clock is not working, the light will stay on
// If on battery power with working clock/MOSFET setup, the d1 mini (and light) will shut off after blinking
// Blinks will repeat once per minute

#include "Wire.h"
#include "uRTCLib.h"

#define URTCLIB_MODEL_DS3231 2

// uRTCLib rtc;
uRTCLib rtc(0x68);



void setup() {

	Serial.begin(9600);
  delay (2000);
	Serial.println("Serial OK");

	Wire.begin();


  rtc.set_model(URTCLIB_MODEL_DS3231);
  rtc.set_rtc_address(0x68);

  rtc.alarmSet(URTCLIB_ALARM_TYPE_1_FIXED_S, 30, 0, 0, 1); // Each minute, at :30 seconds, clock will tell MOSFET to turn on d1 mini
  // RTCLib::alarmSet(uint8_t type, uint8_t second, uint8_t minute, uint8_t hour, uint8_t day_dow);


  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second

  rtc.alarmClearFlag(URTCLIB_ALARM_1); // clear alarm flag shutting power off

}

void loop() {
}
