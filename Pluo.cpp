#include <Arduino.h>
#include <Pluo.h>
#include <RTClib.h>

// TASK: Integrate debug into project for easy end-user troubleshooting.
#define AQUAKIT_PRINT terminal;



// Define static variables.
bool waterZone::_shiftEnabled = false;
int waterZone::_serialPin = 0;
int waterZone::_latchPin = 0;
int waterZone::_clockPin = 0;
int waterZone::_totalBytes = 0;
int* waterZone::_byte = nullptr;

waterZone::waterZone(unsigned int zonePin, unsigned int startTime,
                     unsigned int stopTime, unsigned long daysOfWeek) {

    // Set pin or shift register pin as generic index.
    _index = zonePin;

    // If all schedule parameters are specified, set the schedule.
        // A valid startTime can be 0; a valid stopTime cannot be 0.
    if (stopTime && daysOfWeek) schedule(startTime, stopTime, daysOfWeek);
    // Otherwise, do not set the schedule, and disable the zone.
    else _enabled = false;

    _onVerification = false;

    _manualOverride = false;
    _timedManualOverride = false;
    _tmoStopHour = 0;
    _tmoStopMinute = 0;

}

void waterZone::begin() {

    pinMode(_index, OUTPUT);
    digitalWrite(_index, LOW);

}

void waterZone::begin(int serialPin, int latchPin, int clockPin,
                      int totalBytes) {

    if (! _shiftEnabled) {

        // Shift mode enabled.
        _shiftEnabled = true;

        // Save array size for future operations.
        _totalBytes = totalBytes;
        // Create shift register array on heap.
        _byte = new int [totalBytes];

        // Initiliaze entire array to 0.
        for (int i = 0; i < totalBytes; i++) _byte[i] = 0;

        // Save shift register pins.
        _serialPin = serialPin;
        _latchPin = latchPin;
        _clockPin = clockPin;

        // Set shift register pins to output mode.
        pinMode(serialPin, OUTPUT);
        pinMode(latchPin, OUTPUT);
        pinMode(clockPin, OUTPUT);

        // Set all pins to 0 (off).
        digitalWrite(_latchPin, LOW);
        for (int i = totalBytes - 1; i >= 0; i--)
            shiftOut(_serialPin, _clockPin, MSBFIRST, 0);
        digitalWrite(_latchPin, HIGH);
    }

}

void waterZone::schedule(unsigned int startTime, unsigned int stopTime,
                         unsigned long daysOfWeek, bool enable) {

    // When scheduleed, the zone is enabled by default.
    _enabled = enable;

    // Parse start and end times into hour and minute values.
    _startHour = startTime / 100;
    _startMinute = startTime % 100;
    _stopHour = stopTime / 100;
    _stopMinute = stopTime % 100;

    // Save the scheduled days as an unsigned long.
    _daysofWeekLong = daysOfWeek;

    // Count number of digits in daysOfWeek parameter.
    int dayCount = 1;
    unsigned long countDaysOfWeek = daysOfWeek;
    while (countDaysOfWeek /= 10) dayCount++;

    // Fill _daysOfWeek[] with 0's to remove previous records.
    for(int i = 0; i < 7; i++) _daysOfWeek[i] = 0;
    
    // Read each digit of daysOfWeek and load into _daysOfWeek[] irrigation
    // schedule.
    for(int i = 0; i < dayCount; i++) {
        // Retrieve one digit (scheduled day) from daysOfWeek.
        int digit = daysOfWeek % 10;
        // Go to index of scheduled day in _daysofweek[], set value as 1.
        _daysOfWeek[digit-1] = 1;
        // Remove scheduled day from daysOfWeek.
        daysOfWeek /= 10;
    }
    for(int i = 0; i < 7; i++) Serial.print(_daysOfWeek[i]);

}

void waterZone::schedule(uint16_t startTime,
                         uint16_t frequency, uint8_t duration, 
                         uint16_t subFrequency = 0, uint8_t subDuration = 0) {
    _schedule1Enabled = false;
    _schedule2Enabled = true;
    _schedule3Enabled = false;
    _flowFactor = 1.00;
    

}

unsigned long waterZone::read(int scheduleElement) {

    unsigned long scheduleValue;
    // Start time:
    if (scheduleElement == 0) {
        scheduleValue = _startHour * 100 + _startMinute;
        return scheduleValue;
    }
    // Stop time:
    else if (scheduleElement == 1) {
        scheduleValue = _stopHour * 100 + _stopMinute;
        return scheduleValue;
    }
    // Start hour:
    else if (scheduleElement == 2) {
        scheduleValue = _startHour;
        return scheduleValue;
    }
    // Start minute:
    else if (scheduleElement == 3) {
        scheduleValue = _startMinute;
        return scheduleValue;
    }
    // Stop hour:
    else if (scheduleElement == 4) {
        scheduleValue = _stopHour;
        return scheduleValue;
    }
    // Stop minute:
    else if (scheduleElement == 5) {
        scheduleValue = _stopMinute;
        return scheduleValue;
    }
    // Days of the week:
    else if (scheduleElement == 6) {
        scheduleValue = _daysofWeekLong;
        return scheduleValue;
    }
    else return 0;

}

bool waterZone::run(uint32_t currentUnixTime) {

    DateTime now(currentUnixTime);
    bool returnVal = false;
    // Automatic Event Trigger:
    if (_enabled && _manualOverride == false
        && _daysOfWeek[now.dayOfTheWeek()]) {
        // Auto On:
        if (_onVerification == false && now.hour() == _startHour
            && now.minute() == _startMinute) {
            _on();
            _onVerification = true;
            returnVal = true;
        }
        // Auto Off:
        else if (_onVerification == true && now.hour() == _stopHour
            && now.minute() == _stopMinute) {
            _off();
            _onVerification = false;
            returnVal = true;
        }
    }
    // Automatic Trigger Override Reset:
    // In case of manual override mode, false auto triggers execute to prevent
    // unexpected true auto triggers.
    else if (_enabled && _manualOverride == true
        && _daysOfWeek[now.dayOfTheWeek()-1]) {
        // If manual off occurs during auto on trigger (1 minute window), this
        // prevents the zone from immediately turning back on.
        if (_onVerification == false && now.hour() == _startHour
            && now.minute() == _startMinute) {
            _onVerification = true;
        }
        // 
        else if (_onVerification == true && now.hour() == _stopHour
            && now.minute() == _stopMinute) {
            _onVerification = false;
        }
    }

    // Automatic Timed Event Trigger:
    if (_timedManualOverride == true && now.hour() == _tmoStopHour
        && now.minute() == _tmoStopMinute) {
        off();
        returnVal = true;
    }

    return returnVal;

}

// Manually turn on irrigation zone.
void waterZone::on() {

    _on();
    _manualOverride = true;

}

// Manually turn on irrigation zone with timer to turn off.
void waterZone::on(DateTime now, unsigned int hoursDuration,
                   unsigned int minutesDuration) {

    _on();
    _manualOverride = true;
    DateTime future = (now + TimeSpan(0, hoursDuration, minutesDuration, 0));
    _tmoStopHour = future.hour();
    _tmoStopMinute = future. minute();
    _timedManualOverride = true;

}

// Manually turn off irrigation zone.
void waterZone::off() {

    _off();
    _manualOverride = false;
    _tmoStopHour = 0;
    _tmoStopMinute = 0;
    _timedManualOverride = false;

}

// Enable irrigation zone; this will enable automatic watering as scheduled.
void waterZone::enable() {

    _enabled = true;

}

// Disable irrigation zone; this will disable automatic watering without
// modifying the schedule.
void waterZone::disable() {

    _enabled = false;

}

// Determine whether irrigation zone is on or off.
bool waterZone::isOn() {

    bool state;

    if (_shiftEnabled) {
        int _shiftRegIndex = _index / 8;
        byte _bitIndex = _index - (_shiftRegIndex * 8);
        // TEST: include code to check state of shift register pins.
        state = bitRead(_byte[_shiftRegIndex], _bitIndex);
    }
    else state = digitalRead(_index);

    return state;

}

bool waterZone::isOff() {

    return !isOn();

}

bool waterZone::isEnabled() {

    return _enabled;

}

bool waterZone::isDisabled() {

    return !_enabled;
    
}

void waterZone::factor(float flowFactor) {

    _flowFactor = flowFactor;

}