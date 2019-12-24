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
    _manualStopTime = false;
    _manualStopTime = 0;

}

void waterZone::begin() {

    pinMode(_index, OUTPUT);
    digitalWrite(_index, LOW);

}

void waterZone::begin(int serialPin, int latchPin, int clockPin,
                      int totalBytes) {

    // Enables shift register if not previously enabled.
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

    _intervalEnabled = false;
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
    for (int i = 0; i < 7; i++) _daysOfWeek[i] = 0;
    // Read each digit of daysOfWeek and load into _daysOfWeek[] irrigation
    // schedule.
    for (int i = 0; i < dayCount; i++) {
        // Retrieve one digit (scheduled day) from daysOfWeek.
        int digit = daysOfWeek % 10;
        // Go to index of scheduled day in _daysofweek[], set value as 1.
        _daysOfWeek[digit-1] = 1;
        // Remove scheduled day from daysOfWeek.
        daysOfWeek /= 10;
    }
    for (int i = 0; i < 7; i++) Serial.print(_daysOfWeek[i]);

}

void waterZone::schedule(uint32_t masterStartTime,
                         uint32_t primaryFrequency, uint32_t primaryDuration, 
                         uint32_t secondaryFrequency, 
                         uint32_t secondaryDuration) {

    _intervalEnabled = true;
    _flowFactor = 1.00;
    _masterStartTime = masterStartTime;
    // NOTE: Future, more complex interval patterns (biweekly, monthly, etc.)
    // can use larger arrays (requires overload).
    _primaryPattern = new uint32_t [2];
    _primaryPattern[0] = primaryFrequency;
    _primaryPattern[1] = primaryDuration;
    if (secondaryFrequency && secondaryDuration) {
        _secondaryPattern = new uint32_t [2];
        _secondaryPattern[0] = secondaryFrequency;
        _secondaryPattern[1] = secondaryDuration;
    }
    _previousTrigger = 0;
    _futureTrigger = 0;
    _currentIterator = 0;

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
    bool autoEvent = false;
    if (!_intervalEnabled) {
        // Automatic events:
        if (_enabled && _manualOverride == false
            && _daysOfWeek[now.dayOfTheWeek()]) {
            // Auto On:
            if (_onVerification == false && now.hour() == _startHour
                && now.minute() == _startMinute) {
                _on();
                _onVerification = true;
                autoEvent = true;
            }
            // Auto Off:
            else if (_onVerification == true && now.hour() == _stopHour
                && now.minute() == _stopMinute) {
                _off();
                _onVerification = false;
                autoEvent = true;
            }
        }
        // _onVerification Reset (for auto events that overlap with manual 
        // events):
        else if (_enabled && _manualOverride == true
            && _daysOfWeek[now.dayOfTheWeek()-1]) {
            // If manual off occurs during auto on event (1 minute window),
            // this prevents the zone from immediately turning back on.
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
    }
    // TEST: Interval scheduling
    else if (_intervalEnabled) {
        // Check start date.
        // Calculate next auto event time using start date and current time.
        // Generate a trigger (see timed manual function).
        //     If start date is today before start time, then generate ON
        //     trigger.
        if (_futureTrigger == 0 && now.unixtime() - _masterStartTime >= 0) {
            // FIXME: include calculations for when multiple frequency values exist.
            uint32_t intervalDifference = (now.unixtime() - _masterStartTime) / _primaryPattern[0]; // e.g. (4005 - 2000)/1000 = 2
            _previousTrigger = _masterStartTime + intervalDifference * _primaryPattern[0]; // e.g. 2000 + 2 * 1000 = 4000
            _futureTrigger = _previousTrigger + _primaryPattern[0]; // e.g. 4000 + 1000 = 5000
        }
        if (now.unixtime() >= _futureTrigger) {
            if (_currentIterator % 2 == 0) {
                _on();
                // Save previous trigger.
                _previousTrigger = _futureTrigger;
                _incrementIterator();
                // Set new future trigger ("on" duration).
                _futureTrigger += _primaryPattern[_currentIterator];
                // Set return value.
                autoEvent = true;
            }
            else {
                _off();
                // Save previous trigger as previous START time.
                _previousTrigger = _futureTrigger - _primaryPattern[_currentIterator];
                // Set future trigger equal to previous START time.
                _futureTrigger = _previousTrigger;
                _incrementIterator();
                // Set new future trigger (time until next "on").
                _futureTrigger += _primaryPattern[_currentIterator];
                // Set return value.
                autoEvent = true;
            }
        }
    }
    // Auto Off Timer for Manual On:
    if (_manualStopTime == true && now.unixtime() >= _manualStopTime) {
        off();
        autoEvent = true;
    }
    return autoEvent;

}

// Manually turn on irrigation zone.
void waterZone::on() {

    _on();
    _manualOverride = true;

}

// Manually turn on irrigation zone with timer to turn off.
// TEST: updated timed manual on/off method.
void waterZone::on(uint32_t currentUnixTime, uint8_t hoursDuration,
                   uint8_t minutesDuration) {

    _on();
    DateTime now(currentUnixTime);
    _manualOverride = true;
    DateTime future = (now + TimeSpan(0, hoursDuration, minutesDuration, 0));
    _manualStopTime = future.unixtime();
    _manualStopTime = true;

}

// Manually turn off irrigation zone.
void waterZone::off() {

    _off();
    _manualOverride = false;
    _manualStopTime = 0;
    _manualStopTime = false;

}

// Enable automatic irrigation according to the schedule.
void waterZone::enable() {

    _enabled = true;

}

// Disable automatic irrigation without modifiying the schedule.
void waterZone::disable() {

    _enabled = false;

}

// TEST: Determine whether irrigation zone is on or off.
bool waterZone::isOn() {

    bool state;
    if (_shiftEnabled) {
        int _shiftRegIndex = _index / 8;
        byte _bitIndex = _index - (_shiftRegIndex * 8);
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