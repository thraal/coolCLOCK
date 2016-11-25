/*
Name:		coolClock.ino
Created:	24-Nov-16 16:57:45
Author:	Tom
*/

// defines

#define countof(a) (sizeof(a) / sizeof(a[0]))
#define DHTPIN 6                              // DHT temperature sensor pin
#define DHTTYPE DHT22                         // set DHT to type DHT22 (AM2302)

// libraries

#include <FiniteStateMachine.h>
#include <Adafruit_Sensor.h> // dependency
#include <DHT.h>
#include <DHT_U.h>           // dependency
#include <LiquidCrystal.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <avr/pgmspace.h>    // dependency
#include <Wire.h>            // dependency
#include <RtcDS1307.h>
#include <RBD_Timer.h>
#include <RBD_Button.h>

// global variables: hardware initialization

RtcDS1307 Rtc;                         // real-time clock
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // initialize the library with the interface pins
DHT_Unified dht(DHTPIN, DHTTYPE);      // DHT temperature sensor

RBD::Button leftButton(8);
RBD::Button rightButton(9);
RBD::Button upButton(49);
RBD::Button downButton(47);
RBD::Button setButton(45);
RBD::Button modeButton(41);
RBD::Button alarm1Button(51);
RBD::Button alarm2Button(53);
RBD::Button okButton(43);
RBD::Button snoozeButton(39);

// function prototypes

void timedisplayEnter();
void timedisplayUpdate();
void timedisplayExit();
void configureEnter();
void configureUpdate();
void configureExit();
void alarmEnter();
void alarmUpdate();
void alarmExit();
void alarm1displayUpdate();
void alarm2displayUpdate();
void alarm1setEnter();
void alarm2setEnter();
void alarm1setUpdate();
void alarm2setUpdate();
void alarm1setExit();
void alarm2setExit();

// global variables: function logic

float temperature, humidity; // display value
const float tempoffset = 0;  // offset for temperature calibration
const char initmessage[] = "Alarmklok Initialisatie";
int errorflag = 0;

// Alarms

AlarmId alarmSyncRealTimeClock;
AlarmId alarmSyncTemperature;
AlarmId alarmDisplayTime;

// Timers
RBD::Timer secondsClearTimer;
RBD::Timer secondsDisplayTimer;

// Finite State Machine states

State timedisplay = State(timedisplayEnter, timedisplayUpdate, timedisplayExit); // default display mode
State configure = State(configureEnter, configureUpdate, configureExit);         // clock configure mode
State alarm = State(alarmEnter, alarmUpdate, alarmExit);                         // alarm going off
State alarm1display = State(alarm1displayUpdate);                                // display alarm 1
State alarm2display = State(alarm2displayUpdate);                                // display alarm 2
State alarm1set = State(alarm1setEnter, alarm1setUpdate, alarm1setExit);         // set alarm 1
State alarm2set = State(alarm2setEnter, alarm2setUpdate, alarm2setExit);         // set alarm 2
FSM stateMachine = FSM(timedisplay);                                             // initialize state machine, start state

																				 /*
																				 *  board set-up routine
																				 */

void setup() {
	// enable LCD Display
	lcd.begin(16, 2);              // set up the LCD's number of columns and rows:
	lcd.setCursor(0, 0);
	lcd.print(initmessage);

	// enable DHT temperature sensor
	dht.begin();

	// Debugging
	Serial.begin(9600);
	while (!Serial);              // wait for Arduino Serial Monitor
	serialPrefix();
	Serial.print("Clock booting\n");

	// enable Real-Time Clock
	Rtc.Begin();
	if (!Rtc.IsDateTimeValid())
		errorflag = 1;
	if (!Rtc.GetIsRunning())
		Rtc.SetIsRunning(true);

	// Read and set alarms
	syncRealTimeClock();

	// Function logic timers
	alarmSyncRealTimeClock = Alarm.timerRepeat(600, readRealTimeClock); // sync RTC Clock every 10 minutes
	alarmSyncTemperature = Alarm.timerRepeat(10, syncTemperature);      // sync Temperature every 10 seconds
	alarmDisplayTime = Alarm.timerRepeat(60, displayClock);             // display time update every 60 seconds
}

/*
* Main Board Loop
*/

void loop() {
	Alarm.delay(1); // hit backend alarm logic
	stateMachine.update(); // don't remove :)
}

/*
* States
*/

/*
* Default Display Mode
*/
void timedisplayEnter() {
	serialPrefix();
	Serial.print("STATE: enter timedisplay\n");
	secondsClearTimer.setTimeout(900);
	secondsDisplayTimer.setTimeout(0);
	secondsClearTimer.restart();
	secondsDisplayTimer.restart();
	lcd.clear();
	readTemperature();
	readRealTimeClock();
	displayClock();
	Alarm.enable(alarmSyncRealTimeClock);
	Alarm.enable(alarmSyncTemperature);
	Alarm.enable(alarmDisplayTime);
}

void timedisplayUpdate() {
	if (setButton.onPressed()) stateMachine.transitionTo(configure);

	if (secondsClearTimer.onRestart()) {
		lcd.setCursor(2, 0);
		lcd.print(" ");
		secondsClearTimer.setTimeout(1000);
		secondsClearTimer.restart();
	}

	if (secondsDisplayTimer.onRestart()) {
		lcd.setCursor(2, 0);
		lcd.print(":");
		secondsDisplayTimer.setTimeout(1000);
		secondsDisplayTimer.restart();
	}
}

void timedisplayExit() {
	serialPrefix();
	Serial.print("STATE: exit timedisplay\n");
	Alarm.disable(alarmSyncRealTimeClock);
	Alarm.disable(alarmSyncTemperature);
	Alarm.disable(alarmDisplayTime);
	lcd.clear();
}

/*
* Clock Configure Mode
*/
void configureEnter() {
	serialPrefix();
	Serial.print("STATE: enter configuration\n");
	lcd.clear();
	lcd.print("Instellingen");
}
void configureUpdate() {
	if (setButton.onPressed()) stateMachine.transitionTo(timedisplay);
}
void configureExit() {
	serialPrefix();
	Serial.print("STATE: exit configuration\n");
	lcd.clear();
}

/*
* Alarm going off
*/
void alarmEnter() {

}
void alarmUpdate() {

}
void alarmExit() {

}

/*
* Display and set Alarm1 and Alarm2
*/
void alarm1displayUpdate() {

}
void alarm2displayUpdate() {

}
void alarm1setEnter() {

}
void alarm1setUpdate() {

}
void alarm1setExit() {

}
void alarm2setEnter() {

}
void alarm2setUpdate() {

}
void alarm2setExit() {

}

/*
* Helper functions
*/

void syncTemperature() {
	readTemperature();
	displayTemp();
}

void syncRealTimeClock() {
	serialPrefix();
	Serial.print("METHOD: syncRealTimeClock() - ");
	RtcDateTime now = Rtc.GetDateTime();
	setTime(now.Hour(), now.Minute(), now.Second(), now.Day(), now.Month(), now.Year());
	serialPrefix();
	Serial.print(day());
	Serial.print(".");
	Serial.print(month());
	Serial.print(".");
	Serial.print(year());
	Serial.print("\n");
}

void readRealTimeClock() {
	serialPrefix();
	Serial.print("METHOD: readRealTimeClock()\n");
	syncRealTimeClock();
	clearDisplay();
}

void readTemperature() {
	serialPrefix();
	Serial.print("METHOD: readTemperature() - Temperature: ");
	sensors_event_t temp_event;
	dht.temperature().getEvent(&temp_event);
	temperature = temp_event.temperature + tempoffset;
	dht.humidity().getEvent(&temp_event);
	humidity = temp_event.relative_humidity;
	Serial.print(temperature);
	Serial.print("C");
	Serial.print(", Humidity: ");
	Serial.print(humidity);
	Serial.print("%\n");
}

void displayTemp() {
	serialPrefix();
	Serial.print("METHOD: displayTemp()\n");
	lcd.setCursor(0, 1);
	lcd.print("T:");
	lcd.print(temperature);
	lcd.setCursor(6, 1);
	lcd.print("C V:");
	lcd.print(humidity);
	lcd.setCursor(14, 1);
	lcd.print("%");
}

void clearDisplay() {
	lcd.clear();
	displayDate();
	displayTemp();
}

void displayDate() {
	serialPrefix();
	Serial.print("METHOD: displayDate()\n");
	lcd.setCursor(10, 0);
	printDigits(day());
	lcd.print("/");
	printDigits(month());
}

void displayClock() {
	serialPrefix();
	Serial.print("METHOD: displayClock()\n");
	lcd.setCursor(0, 0);
	printDigits(hour());
	lcd.setCursor(3, 0);
	printDigits(minute());
}

void serialPrefix() {
	serialPrintDigits(hour());
	Serial.print(":");
	serialPrintDigits(minute());
	Serial.print(":");
	serialPrintDigits(second());
	Serial.print(" ");
}

void serialPrintDigits(int digits) {
	if (digits < 10)
		Serial.print('0');
	Serial.print(digits);
}

void printDigits(int digits) {
	if (digits < 10)
		lcd.print('0');
	lcd.print(digits);
}