/*

TODO:     . = WIP     ✓ = completed

    [ ] Integrate debug printing (Serial, Blynk terminal)
    [ ] Fully convert to interval timing
    [ ] Full refactor after interval conversion

*/
#ifndef Pluo_h
#define Pluo_h

#include <Arduino.h>
#include <RTClib.h>

// Values for waterZone.read() arguments
#define START_TIME 0
#define STOP_TIME 1
#define START_HOUR 2
#define START_MINUTE 3
#define STOP_HOUR 4
#define STOP_MINUTE 5
#define DAYS_OF_WEEK 6

class waterZone {

    private:

        // Hardware Info:
        unsigned int _index;        // Contains board pin or shift register pin.
        static bool _shiftEnabled;  // True if using shift register(s).
        static int _serialPin;      // For shift register(s).
        static int _latchPin;       // For shift register(s).
        static int _clockPin;       // For shift register(s).
        static int _totalBytes;     // Number of shift registers.
        static int* _byte;         // Contains byte state of each register.

        // Zone on/off:
        void _on() {
            if (_shiftEnabled) {
                // Calculate which shift register contains the index pin.
                // e.g. 26/8 = 3 (sri)
                int _shiftRegIndex = _index / 8;
                // Calculate which pin (bit) on the specified shift register to
                // be set to true (1).
                // e.g. bit index = 2
                byte _bitIndex = _index - (_shiftRegIndex * 8);
                // e.g. set bit 2 00000200 (least significant bit on left)
                bitSet(_byte[_shiftRegIndex], _bitIndex);
                // Shift out the state (as 1 byte) of each shift register.
                digitalWrite(_latchPin, LOW);
                for (int i = _totalBytes - 1; i >= 0; i--)
                    // e.g. shifted byte is translated into 00200000
                    shiftOut(_serialPin, _clockPin, MSBFIRST, _byte[i]);
                digitalWrite(_latchPin, HIGH);
            }
            else digitalWrite(_index, HIGH);
        }
        void _off() {
            if (_shiftEnabled) {
                // Calculate which shift register contains the index pin.
                int _shiftRegIndex = _index / 8;
                // Calculate which pin (bit) on the specified shift register to
                // be set to false (0).
                byte _bitIndex = _index - (_shiftRegIndex * 8);
                bitClear(_byte[_shiftRegIndex], _bitIndex);
                // Shift out the state (as 1 byte) of each shift register.
                digitalWrite(_latchPin, LOW);
                for (int i = _totalBytes - 1; i >= 0; i--)
                    shiftOut(_serialPin, _clockPin, MSBFIRST, _byte[i]);
                digitalWrite(_latchPin, HIGH);
            }
            else digitalWrite(_index, LOW);
        }

        // Auto Variables:
        unsigned int _startHour;
        unsigned int _startMinute;
        unsigned int _stopHour;
        unsigned int _stopMinute;
        unsigned long _daysofWeekLong;
        bool _daysOfWeek[7];
        bool _enabled;
        bool _onVerification;

        // Interval Auto Variables:
        uint8_t _currentIterator;
        uint32_t _masterStartTime;
        uint32_t _previousTrigger;
        uint32_t _futureTrigger;
        bool _intervalEnabled;
        float _flowFactor;
        uint32_t _startDate;
        uint16_t _startTime;
        uint32_t* _primaryPattern;
        uint8_t _primarySize;
        uint32_t* _secondaryPattern;
        uint8_t _secondarySize;
        // TEST: iterator rollover function
        void _incrementIterator() {
            _currentIterator++;
            if (_currentIterator < _primarySize) return;
            else _currentIterator = 0;
        }

        // Manual Variables:
        bool _manualOverride;
        bool _timedManualOverride;
        uint32_t _manualStopTime;

    public:

        // Construct zone object, set schedule, and enable automatic watering.
        waterZone(unsigned int zonePin, unsigned int startTime = 0,
                  unsigned int stopTime = 0, unsigned long daysOfWeek = 0);
        // Initialize pin for zone.
        void begin();
        // Initialize shift register(s) for all zones.
        void begin(int serialPin, int latchPin, int clockPin,
                   int totalBytes = 1);
        void schedule(unsigned int startTime, unsigned int stopTime,
                      unsigned long daysOfWeek, bool enable = true);
        // TODO: interval timing!!
        // TODO: how do I deal with saving the startDate across resets?
        void schedule(uint32_t masterStartTime,
                      uint32_t primaryFrequency, uint32_t primaryDuration, 
                      uint32_t secondaryFrequency = 0, 
                      uint32_t secondaryDuration = 0);
        void schedule(uint32_t& primaryPattern, uint32_t& secondaryPattern);
        // Return specificied schedule element.
        unsigned long read(int scheduleElement);
        // Check schedule to see if time to water.
        bool run(uint32_t currentUnixTime);
        // REVIEW: does a second run statement work?
        // bool run(DateTime currentTime);
        // Turn irrigation on.
        void on();
        // Turn irrigation on for a duration (in minutes).
        void on(uint32_t currentUnixTime, uint8_t hoursDuration,
                uint8_t minutesDuration);
        // Turn irrigation off.
        void off();
        // Return TRUE if irrigation is in progress.
        bool isOn();
        // Return TRUE if irrigation is not in progress.
        bool isOff();
        // Enable automatic irrigation. (True during object
        // construction if all parameters are provided.)
        void enable();
        // Disable automatic irrigation
        void disable();
        // Return TRUE if automatic irrigation is enabled.
        bool isEnabled();
        // Return TRUE if automatic irrigation is disabled.
        bool isDisabled();
        // Change irrigation flow factor.
        void factor(float flowFactor); // TODO: Test flow factor...other method?
        // TODO: create destructor
        //~waterZone();

};

// The following functions convert other time units to seconds for schedule() 
// arguments:

uint32_t minutes(uint16_t minutes) {
    uint32_t seconds = minutes * 60;
    return seconds;
}

uint32_t hours(uint16_t hours) {
    uint32_t seconds = hours * 60 * 60;
    return seconds;
}

uint32_t days(uint16_t days) {
    uint32_t seconds = days * 24 * 60 *60;
    return seconds;
}

uint32_t weeks(uint16_t weeks) {
    uint32_t seconds = weeks * 7 * 24 * 60 * 60;
    return seconds;
}

#endif