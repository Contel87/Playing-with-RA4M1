#ifndef _STATI_H_
#define _STATI_H_
#include <Arduino.h>

enum stati {st_Menu0, st_Menu1};

struct app_state {
 enum stati current;
 bool first = true;
 int deviceid;
 // aggiungere qui nuove variabili
};

void go(struct app_state *st, enum stati dest);
void Menu0(struct app_state *st);
void Menu1(struct app_state *st);


#endif

