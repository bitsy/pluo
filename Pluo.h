/*

TO DO:     . = WIP     ✓ = completed

    [ ] Integrate debug printing (Serial, Blynk terminal)

*/
#ifndef Pluo_h
#define Pluo_h

#include <Arduino.h>
#include <RTClib.h>

class waterZone {

    private:

        // Hardware Info:
        unsigned int _index;        // Contains board pin or shift register pin.
        static bool _shiftEnabled;  // True if using shift register(s).
        static int _serialPin;      // For shift register(s).
        static int _latchPin;       // For shift register(s).
        static int _clockPin;       // For shift register(s).
        static int _totalBytes;     // Number of shift registers.
        static int * _byte;         // Contains byte state of each register.

        // Hardware Methods:
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

        // Auto Method Variables:
        unsigned int _startHour;
        unsigned int _startMinute;
        unsigned int _endHour;
        unsigned int _endMinute;
        unsigned long _daysofWeekLong;
        bool _daysOfWeek[7];
        bool _enabled;
        bool _onVerification;

        // Manual Method Variables:
        bool _manualOverride;
        bool _timedManualOverride;
        unsigned int _tmoHoursDuration;
        unsigned int _tmoMinutesDuration;
        unsigned int _tmoEndHour;
        unsigned int _tmoEndMinute;

    public:

        // Construct zone object, set schedule, and enable automatic watering.
        waterZone(unsigned int zonePin, unsigned int startTime = 0,
                  unsigned int endTime = 0, unsigned long daysOfWeek = 0);
        // Initialize pin for zone.
        void begin();
        // Initialize shift register(s) for all zones
        void begin(int serialPin, int latchPin, int clockPin,
                   int totalBytes = 1);
        // Change watering schedule to new parameters.
        // TODO: Instead of endtime, use duration?
        void adjust(unsigned int startTime, unsigned int endTime,
                    unsigned long daysOfWeek, bool enable = true);
        // Return specificied schedule element.
        unsigned long read(String scheduleElement);
        // Check schedule to see if time to water.
        bool run(DateTime now);
        // Turn irrigation on.
        void on();
        // Turn irrigation on for a duration (in minutes).
        // TODO: test this method.
        void on(DateTime now, unsigned int hoursDuration,
                unsigned int minutesDuration);
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

        // void end();      // destructor to remove zones

        // NOTE: how to account for DST??? is it even that important?
        // NOTE: Alternative scheduling patterns: pick a day, then daily, weekly,
        // biweekly, monthly, bimonthly, annually, etc.
            // (maybe specify custom pattern in constructor??)

};

// TODO: test macro, maybe replace with integer values since using macro
// aliases.
#define START_TIME "startTime"
#define END_TIME "endTime"
#define START_HOUR "endHour"

#endif