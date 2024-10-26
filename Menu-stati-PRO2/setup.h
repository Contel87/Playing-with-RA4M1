#ifndef _SETUP_H_
#define _SETUP_H_
#include <Arduino.h>


//--------------Buttons
#define DEC_B          4       // Decrement button Pin
#define INC_B          5       // Increment button Pin
#define ENT_B          6       // Enter button Pin

//---------------Encoder
#define encoderPinA 2          // Encoder CW
#define encoderPinB 3          // Encoder CCW

typedef enum stati {
  st_Menu0,       // Associated to Menu0
  st_Menu1,       // Associated to Menu1
  st_Menu2,       // Associated to Menu1
  elementsCounter // Use this enum element as the last (used as enum elements counter)
  } menu_num_t; // Every menu page has an enum label ...

struct variables {
 enum stati current;
 bool first = true;
 int8_t pos = 0;             // Encoder position (CW = 1 , CCW = -1)
 uint16_t deviceID;
 // aggiungere qui nuove variabili
};

//variables *info = &var;

void set48MhzClock();   // Set external 48Mhz quartz as MOSC
void rtcSOSC(); // Set external 32768Hz SOSC
void encButton();
void myMenu0(struct variables *var);                 // Screen Menu 0
void myMenu1(struct variables *var);                 // Screen Menu 1
void myMenu2(struct variables *var);                 // Screen Menu 2

void testdrawbitmap(void);



#endif

