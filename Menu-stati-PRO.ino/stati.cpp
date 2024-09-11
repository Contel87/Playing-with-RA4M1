/*Implementazione*/
#include "stati.h"
#include <Arduino.h>


void go(struct variables *va, enum menu_list dest){
  va->current = dest;
  va->first = true;
}

void Menu0(struct variables *va){
if(va->first){
 Serial.println("Menu0");
 va->deviceid = 1234;
 va->first = false;
 }

delay(1000);
go(va, st_Menu1); // Move to another state

}



void Menu1(struct variables *va){
if(va->first){
 Serial.println("Menu1");
 Serial.print("devid: ");Serial.println (va->deviceid); // String(st->deviceid)
 va->first = false;
 }

delay(1000);
go(va, st_Menu0); // Move to another state

}

