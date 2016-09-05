/*

  display.h
  
*/

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <stdint.h>

void display_Init(void);
void display_Write(const char *str);
void display_WriteUnsigned(uint32_t v);


#endif