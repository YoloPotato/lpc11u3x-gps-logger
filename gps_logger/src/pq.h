/*

  pq.h
  
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


#ifndef _PQ_H
#define _PQ_H

#include <float.h>
#include <math.h>
#include "gps.h"
#include "crb.h"


#define PH_ERR_STATISTICS

#ifdef OLD
struct _pq_entry_struct
{
  gps_pos_t pos;
  //uint32_t timestamp;	/* seconds since 1.1.2000 */
  //uint16_t millisecond;	/* milliseconds of the timestamp */
};
typedef struct _pq_entry_struct pq_entry_t;
#endif

struct _pq_interface_struct
{
  gps_pos_t pos;
  gps_float_t speed_in_knots;
  gps_float_t true_course;			/* 0.0: North, 90.0: East */
  gps_float_t magnetic_variation;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t millisecond;
};
typedef struct _pq_interface_struct pq_interface_t;

#define PQ_LEN 24

struct _pq_struct
{
  crb_t crb;
  pq_interface_t interface;
  
  uint32_t processed_sentences;
#ifdef PH_ERR_STATISTICS
  uint32_t processed_gprmc;
  uint32_t parser_error_gprmc;
  uint32_t valid_gprmc;
  uint32_t invalid_gprmc;
  uint32_t processed_gpgga;
  uint32_t parser_error_gpgga;
  uint32_t valid_gpgga;
  uint32_t invalid_gpgga;
#endif
  uint8_t gps_quality;	/* GPS quality from GPGGA record */
  uint8_t sat_cnt;	/* satellites in use (GPGGA record) */
  //uint8_t cnt;		/* entries in the queue */	
  uint8_t digit_cnt;	/* number of fraction digits of lon/lat from the gps receiver */

  uint8_t pos_is_neg;	/* temp variable for gps_float_t conversion */
  uint8_t pos_minutes;	/* temp variable for gps_float_t conversion 0..59 */
  gps_float_t pos_minutes_frac;	/* same as pos_minutes, but including the fraction */
  uint16_t pos_fraction;	/* 0...999 */
  uint16_t pos_degree;	/* temp variable for gps_float_t conversion */
  
  //pq_entry_t queue[PQ_LEN];
  char last_unknown_msg[8];
};
typedef struct _pq_struct pq_t;

void pq_Init(pq_t *pq);
void pq_AddChar(pq_t *pq, uint8_t c);
//void pq_DeleteFirst(pq_t *pq);
//void pq_AddInterfaceValuesToQueue(pq_t *pq);
//pq_entry_t *pq_GetLatestEntry(pq_t *pq);
void pq_ResetParser(pq_t *pq);
int16_t pq_GetCurr(pq_t *pq);
int16_t pq_GetNext(pq_t *pq);
uint8_t pq_SkipSpace(pq_t *pq);
uint8_t pq_GetNum(pq_t *pq, uint32_t *num, uint8_t *digit_cnt);
uint8_t pq_GetFloat(pq_t *pq, gps_float_t *f);
uint8_t pq_GetLonLatFloat(pq_t *pq, gps_float_t *f);
const char *pq_GetStr(pq_t *pq);
uint8_t pq_ParseGPRMC(pq_t *pq);
uint8_t pq_ParseGPGGA(pq_t *pq);
uint8_t pq_ParseSentence(pq_t *pq);

void pq_itoa(char *s, uint16_t x, uint8_t cnt);
void pq_FloatToDegreeMinutes(pq_t *pq, gps_float_t f) __attribute__((noinline));
void pq_DegreeMinutesToStr(pq_t *pq, uint8_t is_lat, char *s);
//void pq_FloatToStr(gps_float_t f, char *s);
void pq_FloatToStr(gps_float_t f, uint8_t is_lat, char *s);


#endif 

