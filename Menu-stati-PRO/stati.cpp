/*Implementazione*/
#include "stati.h"
#include <Arduino.h>


void go(struct app_state *st, enum stati dest){
  st->current = dest;
  st->first = true;
}

void Menu0(struct app_state *st){
if(st->first){
 Serial.println("Menu0");
 st->first = false;
 }

st->deviceid = 1234;
delay(1000);
go(st, st_Menu1);

}



void Menu1(struct app_state *st){
if(st->first){
 Serial.println("Menu1");
 st->first = false;
 }

Serial.print("devid: ");Serial.println (st->deviceid); // String(st->deviceid)
delay(1000);
go(st, st_Menu0);


}

