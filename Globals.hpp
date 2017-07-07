#ifndef GLOBALS_HPP
#define GLOBALS_HPP
#include "Waiter.hpp"
#include <math.h>
#include <Ticker.h>

extern const int RED = 15;
extern const int GREEN = 12;
extern const int BLUE = 13;
extern const int LDR = 0xA0;
extern const int BUTTON = 4;
extern const int FANSPEED = 16;
extern const int FANDATA = 14;

extern const int btnjumpignore = 50;
extern const int shortpress = 800;
extern const int longpress = 2000;
extern const int tvok = 2100;

extern void setRGB(int, int, int);

extern void setFan(unsigned short);

#endif
