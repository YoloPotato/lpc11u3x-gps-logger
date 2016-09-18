/*

  gps.h

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

#ifndef _GPS_H
#define _GPS_H

#include <stdint.h>

/*
  gps_float_t is "float" which has 23 bits resolution.
  these are about 7 (6.9) decimal digits.
  for -180 to 180, 4 digits remain for the decimal fraction
  this is a resolution of about 10m
  
  GPS_MODF stores the integral part in f and returns the fractional part  
*/

//typedef float gps_float_t;
//#define GPS_MODF(x,f)	modff((x),(f))
//#define GPS_FLOAT_MAX	FLT_MAX

typedef double gps_float_t;
#define GPS_MODF(x,f)	modf((x),(f))
#define GPS_FLOAT_MAX	DBL_MAX

struct _gps_pos_struct
{
  gps_float_t latitude;
  gps_float_t longitude;
  gps_float_t altitude;
  uint32_t	time;		/* seconds since 1.1.2000 */
  uint8_t year;			/* since 2000 */
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t sat;
  volatile uint8_t is_altitude_update;	/* set to 1 if the altitude and sat was updated */
  volatile uint8_t is_pos_and_time_update; 
};
typedef struct _gps_pos_struct gps_pos_t;


void gps_ltoa(char *s, uint32_t x, uint8_t cnt);
void gps_float_to_str(gps_float_t f, char *s);


#endif
