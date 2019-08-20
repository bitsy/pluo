# Pluo
An Arduino library that provides tools to create agricultural irrigation systems for personal or professional use.

## Principals of Design

## Keywords
`waterZone` constructs zone object.
`begin()` initializes pin for zone.
`adjust()` changes watering schedule.
`read()` returns specific schedule parameter.
`run()` checks schedule to see if time to water and should be placed in `loop()`.
`on()` turns irrigation on.
`off()` turns irrigation off.
`isOn()` returns `TRUE` if irrigation is in progress.
`isOff()` returns `TRUE` if irrigation is not in progress.
`enable()` enables automatic irrigation.
`disable()` disables automatic irrigation.
`isEnabled()` returns `TRUE` if automatic irrigation is enabled.
`isDisabled()` returns `TRUE` if automatic irrigation is disabled.