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
int * waterZone::_byte = nullptr;

waterZone::waterZone(unsigned int zonePin, unsigned int startTime,
                     unsigned int endTime, unsigned long daysOfWeek) {

    // Set pin or shift register pin as generic index.
    _index = zonePin;

    // If all schedule parameters are specified, set the schedule.
        // A valid startTime can be 0; a valid endTime cannot be 0.
    if (endTime && daysOfWeek) adjust(startTime, endTime, daysOfWeek);
    // Otherwise, do not set the schedule, and disable the zone.
    else _enabled = false;

    _onVerification = false;

    _manualOverride = false;
    _timedManualOverride = false;
    _tmoEndHour = 0;
    _tmoEndMinute = 0;

}

void waterZone::begin() {

    pinMode(_index, OUTPUT);
    digitalWrite(_index, LOW);

}

void waterZone::begin(int serialPin, int latchPin, int clockPin,
                      int totalBytes) {

    if (! _shiftEnabled) {
        //Serial.println("[DEBUG] Initialize shift register.");

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

void waterZone::adjust(unsigned int startTime, unsigned int endTime,
                       unsigned long daysOfWeek, bool enable) {

    // When adjusted, the zone is enabled by default.
    _enabled = enable;

    // Parse start and end times into hour and minute values.
    _startHour = startTime / 100;
    _startMinute = startTime % 100;
    _endHour = endTime / 100;
    _endMinute = endTime % 100;

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
    Serial.println("");

}

unsigned long waterZone::read(String scheduleElement) {

    unsigned long scheduleValue;
    if (scheduleElement == "startTime") {
        scheduleValue = _startHour * 100 + _startMinute;
        return scheduleValue;
    }
    else if (scheduleElement == "endTime") {
        scheduleValue = _endHour * 100 + _endMinute;
        return scheduleValue;
    }
    else if (scheduleElement == "startHour") {
        scheduleValue = _startHour;
        return scheduleValue;
    }
    else if (scheduleElement == "startMinute") {
        scheduleValue = _startMinute;
        return scheduleValue;
    }
    else if (scheduleElement == "endHour") {
        scheduleValue = _endHour;
        return scheduleValue;
    }
    else if (scheduleElement == "endMinute") {
        scheduleValue = _endMinute;
        return scheduleValue;
    }
    else if (scheduleElement == "daysOfWeek") {
        scheduleValue = _daysofWeekLong;
        return scheduleValue;
    }
    else return 0;

}

bool waterZone::run(DateTime now) {

    //Serial.println(String(now.hour()) + ":" + String(now.minute()) + " day: "
    //    + String(now.dayOfTheWeek()) + " " + String(_manualOverride));
    bool returnVal = false;
    // Automatic Event Trigger:
    if (_enabled && _manualOverride == false
        && _daysOfWeek[now.dayOfTheWeek()]) {
        // Auto On:
        if (_onVerification == false && now.hour() == _startHour
            && now.minute() == _startMinute) {
            _on();
            _onVerification = true;
            Serial.println("[DEBUG] auto on");
            returnVal = true;
        }
        // Auto Off:
        else if (_onVerification == true && now.hour() == _endHour
            && now.minute() == _endMinute) {
            _off();
            _onVerification = false;
            Serial.println("[DEBUG] auto off");
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
            //Serial.println("[DEBUG] Set onVer TRUE");
        }
        // 
        else if (_onVerification == true && now.hour() == _endHour
            && now.minute() == _endMinute) {
            _onVerification = false;
            //Serial.println("[DEBUG] Set onVer FALSE");
        }
    }

    // Automatic Timed Event Trigger:
    if (_timedManualOverride == true && now.hour() == _tmoEndHour
        && now.minute() == _tmoEndMinute) {
        off();
        //Serial.println("[DEBUG] timed manual off");
        returnVal = true;
    }

    return returnVal;

}

// Manually turn on irrigation zone.
void waterZone::on() {

    _on();
    _manualOverride = true;

    //Serial.println("[DEBUG] manual on");

}

// Manually turn on irrigation zone with timer to turn off.
void waterZone::on(DateTime now, unsigned int hoursDuration,
                   unsigned int minutesDuration) {

    _on();
    _manualOverride = true;
    DateTime future = (now + TimeSpan(0, hoursDuration, minutesDuration, 0));
    _tmoEndHour = future.hour();
    _tmoEndMinute = future. minute();
    _timedManualOverride = true;

    //Serial.println("[DEBUG] timed manual on");

}

// Manually turn off irrigation zone.
void waterZone::off() {

    _off();
    _manualOverride = false;
    _tmoEndHour = 0;
    _tmoEndMinute = 0;
    _timedManualOverride = false;

    //Serial.println("[DEBUG] manual off");

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