/*
Menu a stati con encoder rotativo e 3 pulsanti.
L'encoder e' collegato ai pin 2 (P105) e 3(P104) della UNO R4;
i pulsanti sono collegati agli ingressi 4(P103) - 5(P102) - 6(P106) 

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
*/

//-------------Menu
#include "setup.h"
variables var;

void setup() {
  // Uncommet two line below only if board has external crystal resonators like WeAct Studio RA4M1.
  // If use Arduino UNO R4 NOT call set48MhzClock() and rtcSOSC() funct.
  set48MhzClock();                         // Set 48Mhz main clock with external 16Mhz chrystal osc and PLL
  rtcSOSC();                               // Select External 32.768Khz quartz

  pinMode(encoderPinA, INPUT_PULLUP);      // Buttons are directly connected to GND
  pinMode(encoderPinB, INPUT_PULLUP);      // Buttons are directly connected to GND
  pinMode(DEC_B, INPUT_PULLUP);            // Dec.button
  pinMode(INC_B, INPUT_PULLUP);            // Inc.button and 30 second Adjust
  pinMode(ENT_B, INPUT_PULLUP);            // Ent. button
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderA, CHANGE);  // Call interrupt when turn encoder CW
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderB, CHANGE); // Call interrupt when turn encoder CCW
  Serial.begin(9600);                      // Initialize Serial communication
  delay(2000);                             // Arduino UNO R4 need delay to open Serial
  Serial.println(__FILE__);
}


void loop() {
switch(var.current){     // Select properly menu page with switch - case conditions ...
    case st_Menu0:       // If enum stati current == stMenu0
      myMenu0(&var);       // Enter inside Menu0 page
      checkEnc();        // Check if encoder rotate
    break;

    case st_Menu1:      // If enum stati current == stMenu1
      myMenu1(&var);    // Enter inside Menu1 page
      checkEnc();
    break;

    case st_Menu2:       // If enum stati current == stMenu2
      myMenu2(&var);     // Enter inside Menu2 page
      checkEnc();
    break;
 
  }
}



void checkEnc(){ // Check encoder rotations and move inside menu pages
  static int8_t value, oldvalue = 0;
  int8_t maxValue = stati::elementsCounter -1; // Number of elements inside enum stati
  int8_t minValue = 0; // Min value
  static uint32_t pm = millis();
  uint32_t cm = millis();
  if (cm - pm >= 200){ // debouncing time
    pm = cm;
  if (var.pos == 1) {
    ++value;
  }
  if (var.pos == -1) {
    --value;
  }
  if (value != oldvalue) {
    if (value >= maxValue) value = maxValue;
    if (value <= minValue) value = minValue;
    if (value != oldvalue){
    var.first = true;
    var.current = static_cast<menu_num_t>(value); //convert encoder position (int) to enum element (current)
    }
  }
  oldvalue = value;
  var.pos = 0;
  }  
}


void doEncoderA() {
  // look for a low-to-high on channel A
  if (digitalRead(encoderPinA) == HIGH) {
    // check channel B to see which way encoder is turning
    if (digitalRead(encoderPinB) == LOW) {
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  } else {// look for a high-to-low on channel A
    if (digitalRead(encoderPinB) == HIGH) {// check channel B to see which way encoder is turning
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  }  
}

void doEncoderB() {
  // look for a low-to-high on channel B
  if (digitalRead(encoderPinB) == HIGH) {
    // check channel A to see which way encoder is turning
    if (digitalRead(encoderPinA) == HIGH) {
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  } else { // Look for a high-to-low on channel B
    // check channel B to see which way encoder is turning
    if (digitalRead(encoderPinA) == LOW) {
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  }
}

