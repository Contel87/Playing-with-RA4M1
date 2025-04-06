
/*
 * Data-Watchpoint-and-Trace-Unit and Arduino UNO R4
 * For more informations: https://developer.arm.com/documentation/ddi0439/b/Data-Watchpoint-and-Trace-Unit/DWT-functional-description
 * @author   Luigi Conte - https://github.com/Contel87
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "core_cm4.h" // Cortex M4 core



typedef enum
{
    SYS_DELAY_UNITS_SECONDS      = 48000000, ///< Requested delay amount is in seconds
    SYS_DELAY_UNITS_MILLISECONDS = 48000,    ///< Requested delay amount is in milliseconds
    SYS_DELAY_UNITS_MICROSECONDS = 48        ///< Requested delay amount is in microseconds
} sys_delay_units_t;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  setupCYCCNT();
  
  //Esempio di uso del contatore per contare i cicli di clock ...
  Serial.println("Invio il comando per accendere il led ...");
  DWT->CYCCNT = 0; // reset counter
  digitalWrite(LED_BUILTIN, HIGH);
  uint32_t power_on = DWT->CYCCNT; //Check counter value
  Serial.print("Il led si e' acceso dopo: ");Serial.print(power_on);Serial.println(" cicli di clock");
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
 digitalWrite(LED_BUILTIN, HIGH);
 delay_TIME(1, SYS_DELAY_UNITS_SECONDS);
 digitalWrite(LED_BUILTIN, LOW);
 delay_TIME(1, SYS_DELAY_UNITS_SECONDS);
}

void delay_TIME(uint32_t delay, sys_delay_units_t unit)
{
    uint32_t SumTime = delay * unit; //Calculate the total delay time (in clock ticks)
    DWT->CYCCNT = 0; // reset counter
    while ((DWT->CYCCNT) <= SumTime){}; // Wait for the counter to reach the set time
}

void setupCYCCNT() {
  /* DEMCR is a 32-bit Debug Exception and Monitor Control Register ...
   * This provides Vector Catching and Debug Monitor Control.
   * This register contains a bit named TRCENA witch enable the use of a TRACE
   * For more informations about CYCCNT counter https://developer.arm.com/documentation/ddi0403/latest/
  */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // TRCENA enable the use of a TRACE
  ITM->LAR = 0xc5acce55; // Unlock access to ITM registers 
  DWT->CYCCNT = 0; // Reset counter
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // CYCCNTENA Enables the Cycle counter
}