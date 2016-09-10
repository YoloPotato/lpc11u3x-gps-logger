/*

  display.h
  
*/

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "gps.h"
#include <stdint.h>

void display_Init(void);
void display_Write(const char *str);
void display_WriteUnsigned(uint32_t v);
void display_WriteGpsFloat(gps_float_t f);


#endif