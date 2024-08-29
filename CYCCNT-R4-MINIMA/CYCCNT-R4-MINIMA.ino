/* UNO R4 MINIMA ... Example to use CYCCNT counter to counter cycles in a block of code 

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/


void setup() {
  Serial.begin(9600);
  delay(2000);
  DWT->CYCCNT = 0;
  
  // ------ Insert below the code to check
  //delay(100);
  delayMicroseconds(1000);
  // -------------------------------------

  uint32_t milliSec = DWT->CYCCNT;
  Serial.print(milliSec);Serial.print(" cycles");Serial.print(" ---> ");
  Serial.print((float)(milliSec / 1000) / 48);Serial.println(" milliSeconds");
}

void loop() {}

void setupCYCCNT() {
  /* DEMCR is a 32-bit Debug Exception and Monitor Control Register ...
This provides Vector Catching and Debug Monitor Control.
This register contains a bit named TRCENA witch enable the use of a TRACE
For more informations about CYCCNT counter https://developer.arm.com/documentation/ddi0403/latest/
*/
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // TRCENA enable the use of a TRACE
  ITM->LAR = 0xc5acce55; // Unlock access to ITM registers 
  DWT->CYCCNT = 0; // Reset counter
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // CYCCNTENA Enables the Cycle counter
}
