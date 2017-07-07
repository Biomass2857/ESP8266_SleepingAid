#include "Globals.hpp"

extern void setRGB(int r, int g, int b)
{
  analogWrite(RED, r * 4);
  analogWrite(GREEN, g * 4);
  analogWrite(BLUE, b * 4);
}

extern void setFan(unsigned short speed)
{
  analogWrite(FANSPEED, 1023 * speed / 100);
}
