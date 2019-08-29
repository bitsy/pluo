# Pluo
Pluo is an Arduino library that provides tools to create agricultural irrigation systems for personal or professional use.

## Principals of Design
### Modularity
Building a complex system should require simple building blocks. This library aims to provide an easily expandable platform with all the resources you need and none of the ones you don't.

### Scalability
Build a project to water that pesky fern in your bedroom, or design an industrial system to irrigate your nursery. You can build a robust irrigation system with a virtually unlimited[*](#Notes) number of zones.

### Accessibility
Free code is great. But, if you can't read it, *who cares?* This library prioritizes readability both in its API and its back-of-house implementation. It is provided free of charge for *anyone* to use and free of nonsensical language for *anyone* to read.
___
## Installation & Dependencies
Pluo currently relies on [Adafruit](https://www.adafruit.com)'s [RTClib](https://github.com/adafruit/RTClib) to function correctly.
### Arduino IDE
(under development)
### PlatformIO (VSCode)
(under development)
### Via Command Line
(under development)
___
## Basic Usage & Examples
### Program Structure
To create an irrigation zone, use the following format:
```
waterZone myZone(13, 530, 605, 1357);
```
The first parameter is the pin number that will be used to toggle the zone on or off. The next two parameters are the start and stop times (24-hour format), formatted as positive integers (e.g. `myZone` begins irrigation at 5:30AM and stops at 6:05AM). The final parameter specifies irrigation days as a positive integer (e.g. `myZone` would irrigate on Sunday, Tuesday, Thursday, and Saturday). *NOTE: You may initialize your zones using only pin numbers, and use* `adjust()` *to set the irrigation schedule in another section of your code.*

The basic structure of an automatic sprinkler system program might look like this:
```
waterZone myZone(13, 530, 605, 1357);

setup() {
    myZone.begin();
}

loop() {
    myZone.run();
}
```
View the [basic example](examples/basic.ino) sketch for further implementation details.

### Advanced Features
Alternatively, you may initialize zones in an array:
```
waterZone myZone[3] = {
    {33},   // myZone[0] on pin 33
    {27},   // myZone[1] on pin 27
    {12},   // myZone[2] on pin 12
};
```
In this case, you must use a `for` loop to iterate through `begin()` and `run()` for each zone. This is a very efficient way to manage multiple zones. For a more in-depth understanding, view the example sketch [basicArray](examples/basicArray.ino).

In addition, Pluo also supports 8-bit SIPO shift registers. You can initialize a shift register by supplying the serial, latch, and clock pins to `begin()`:
```
myZone.begin(11, 12, 13);
```
View [basicShift](examples/basicShift.ino) sketch for more details.
___
## Keywords
`waterZone` constructs zone object.\
`begin()` initializes the pin mode for the zone. Optional arguments can be used to initialize a shift register.\
`adjust()` changes the watering schedule.\
`read()` returns the value of a specified schedule parameter.\
`run()` checks the schedule to see if time to water.
`on()` turns irrigation on.\
`on(<duration>)` turns irrigation on for specified duration (in minutes). *NOTE: This feature is still experimental and may not function as expected.*\
`off()` turns irrigation off.\
`isOn()` returns `TRUE` if irrigation is in progress.\
`isOff()` returns `TRUE` if irrigation is not in progress.\
`enable()` enables automatic irrigation.\
`disable()` disables automatic irrigation.\
`isEnabled()` returns `TRUE` if automatic irrigation is enabled.\
`isDisabled()` returns `TRUE` if automatic irrigation is disabled.

###### Notes
*Limited, of course, only by available hardware, fiscal assets, and/or imagination!