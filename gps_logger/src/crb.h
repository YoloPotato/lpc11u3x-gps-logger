/*

  crb.h

  char ring buffer 

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


#ifndef _CRB_H
#define _CRB_H

#include <stdint.h>

#define CRB_LEN (256*4)

struct _crb_stuct
{
  volatile uint16_t start;
  volatile uint16_t good_end;
  volatile uint16_t end;
  volatile uint8_t is_full;
  volatile uint8_t is_wait_for_dollar;	/* only updated by crb_AddChar() */
  uint8_t buf[CRB_LEN];

  uint16_t curr_pos;
};
typedef struct _crb_stuct crb_t;

int16_t crb_GetInit(crb_t *crb);
int16_t crb_GetCurr(crb_t *crb);
int16_t crb_GetNext(crb_t *crb);

void crb_Init(crb_t *crb);
uint8_t crb_IsSentenceAvailable(crb_t *crb);
void crb_DeleteSentence(crb_t *crb);
void crb_AddChar(crb_t *crb, uint8_t c);
void crb_AddStr(crb_t *crb, const char *str);


#endif
