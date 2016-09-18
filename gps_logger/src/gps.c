/*

  gps.c
  
  
  
*/

#include <float.h>
#include <math.h>
#include <stdint.h>
#include "gps.h"

/* 
  writes cnt + 1 chars to s, including '\0' 

  maybe use rom div procedures
  romdiv files are included in AN11732.zip
  
*/
void gps_ltoa(char *s, uint32_t x, uint8_t cnt)
{
  uint16_t c;
  s[cnt] = '\0';
  while( cnt > 0 )
  {
    cnt--;
    c = x % 10UL;
    x /= 10UL;
    c += '0';
    s[cnt] = c;
  }
  
}


/*
  similar to pq_FloatToStr, but uses "-" instead of direction chars
*/
void gps_float_to_str(gps_float_t f, char *s)
{
  gps_float_t g;

  if ( f < (gps_float_t)0 )
  {
    f = -f;
    *s = '-';
    s++;
  }


  f = GPS_MODF(f,&g);
  gps_ltoa(s, (uint32_t)g, 3);
  s[3] = '.';

  if (sizeof(gps_float_t)  < 6 )
  {
    f *= (gps_float_t)10000;
    f = GPS_MODF(f,&g);
    gps_ltoa(s+4, (uint32_t)g, 4);
    s[8] = '\0';
  }
  else
  {
    f *= (gps_float_t)1000000;
    f = GPS_MODF(f,&g);
    gps_ltoa(s+4, (uint32_t)g, 6);
    s[10] = '\0';
  }
}



