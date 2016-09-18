/*

  gps.c
  
  LPC11U3x GPS Logger (https://github.com/olikraus/lpc11u3x-gps-logger)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
  
  
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



