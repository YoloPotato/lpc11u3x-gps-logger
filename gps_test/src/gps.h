/*

  gps.h
  
*/

#ifndef _GPS_H
#define _GPS_H

#include <stdint.h>

/*
  gps_float_t is "float" which has 23 bits resolution.
  these are about 7 (6.9) decimal digits.
  for -180 to 180, 4 digits remain for the decimal fraction
  this is a resolution of about 10m
*/
typedef float gps_float_t;

/* GPS_MODF stores the integral part in f and returns the fractional part */
#define GPS_MODF(x,f)	modff((x),(f))
#define GPS_FLOAT_MAX	FLT_MAX


struct _gps_pos_struct
{
  gps_float_t latitude;
  gps_float_t longitude;
  gps_float_t altitude;
  uint32_t	time;		/* seconds since 1.1.2000 */
};
typedef struct _gps_pos_struct gps_pos_t;


void gps_ltoa(char *s, uint32_t x, uint8_t cnt);
void gps_float_to_str(gps_float_t f, char *s);


#endif
