#ifndef _STATI_H_
#define _STATI_H_
#include <Arduino.h>

enum menu_list {st_Menu0, st_Menu1}; // Stati che compongono il menu

struct variables {
 enum menu_list current;
 bool first = true;
 int deviceid;
 // aggiungere qui nuove variabili
};

void go(struct variables *va, enum menu_list dest);
void Menu0(struct variables *va);
void Menu1(struct variables *va);


#endif

