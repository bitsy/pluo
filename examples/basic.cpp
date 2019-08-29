/*
basic.cpp

Created by Samuel Witt on 08/25/19.
Copyright © 2019 Samuel Witt. All rights reserved.
*/

#include <Arduino.h>
#include <Pluo.h>

waterZone myZone(13, 530, 605, 1357);

// This code runs one time during startup.
void setup() {
    
    myZone.begin();
    
}

// This code runs indefinitely after startup.
void loop() {
    
    myZone.run();
    
}