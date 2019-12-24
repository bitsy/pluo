#include <Arduino.h>
#include <Pluo.h>
#include <RTClib.h>

// REVIEW: check example for errors and test with circuit.

// Implement the RTC library of your choice. (This example uses Adafruit's
// RTCLib and creates a soft RTC.)
RTC_Millis RTC;
DateTime now;

// Initialize multiple irrigation zones, providing the pin number, the start 
// time, the stop time, and the days of the week that each zone should run. 
// (Each zone can be optionally initialized with only pin numbers and scheduled 
// later.)
const int numberOfZones = 8;
waterZone myZone[numberOfZones] = {
    {1},
    {2},
    {3},
    {4},
    {5},
    {6},
    {7},
    {8}
};

// Save shift register pins.
int serialPin = 11;
int latchPin = 8;
int clockPin = 12;

// This code runs one time during startup.
void setup() {

    // Set the RTC time; this example uses the compile time.
    RTC.begin(DateTime(__DATE__, __TIME__));
    // Always place begin() in setup() to initialize pins for each zone.
    for (int i = 0; i < numberOfZones; i++)
        myZone[numberOfZones].begin(serialPin, latchPin, clockPin);

}

// This code runs indefinitely after startup.
void loop() {

    // Get the current time.
    now = RTC.now();
    // Always place run() in loop(), and pass the current time in seconds (unix 
    // time) to each zone.
    for (int i = 0; i < numberOfZones; i++)
        myZone[numberOfZones].run(now.unixtime());
    // RECOMMENDATION: when using run(), avoid excessive use of the delay() 
    // function...state machines are much nicer anyways!

}