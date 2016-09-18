/*

  crb.c

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

#include <stddef.h>
#include <string.h>
#include "crb.h"

void crb_Init(crb_t *crb)
{
  memset(crb, 0, sizeof(crb_t));
  crb->is_wait_for_dollar = 1;
}


uint8_t crb_IsEmpty(crb_t *crb)
{
  if ( crb->start == crb->end )
    return 1;
  return 0;
}

uint8_t crb_IsSentenceAvailable(crb_t *crb)
{
  if ( crb->start != crb->good_end )
    return 1;
  return 0;
}

void crb_DeleteSentence(crb_t *crb)
{
  uint16_t next_start;
  if ( crb_IsSentenceAvailable(crb) == 0 )
    return;
  
  next_start = crb->start;
  for(;;)
  {
    next_start++;
    if ( next_start >= CRB_LEN )
      next_start = 0;
    
    if ( next_start == crb->good_end || crb->buf[next_start] == '$' )
    {
      break;
    }

    /* continue to increase next_start */
  }
  
  crb->start = next_start;
  crb->is_full = 0;
}

void crb_AddChar(crb_t *crb, uint8_t c)
{
  uint16_t new_end;
  if ( crb->is_full != 0 )
    return;
  if ( crb->is_wait_for_dollar != 0 )
  {
    if ( c != '$' )
      return;
    crb->is_wait_for_dollar = 0;
    /* continue normally */
  }
  
  /* increment end of ring buffer */
  new_end = crb->end;
  new_end++;
  if ( new_end >= CRB_LEN )
    new_end = 0;
  
  /* check if there will be a buffer overflow */
  if ( new_end == crb->start )
  {
    crb->is_full = 1;
    crb->is_wait_for_dollar = 1;
    crb->end = crb->good_end;
    return;
  }
  
  /* new_end has been calculated, no overflow will be there */
  if ( c == '$' )
  {
    crb->good_end = crb->end;
  }
  
  crb->buf[crb->end] = c;
  crb->end = new_end;
}

void crb_AddStr(crb_t *crb, const char *str)
{
  while( *str != '\0' )
  {
    crb_AddChar(crb, *str);
    str++;
  }
}

/*================================*/

int16_t crb_GetInit(crb_t *crb)
{
  crb->curr_pos = crb->start;
  return crb->buf[crb->curr_pos];
}

int16_t crb_GetCurr(crb_t *crb)
{
  if ( crb_IsSentenceAvailable(crb) == 0 )
    return -1;
  return crb->buf[crb->curr_pos];
}

int16_t crb_GetNext(crb_t *crb)
{
  crb->curr_pos++;
  if ( crb->curr_pos >= CRB_LEN )
    crb->curr_pos = 0;
  return crb->buf[crb->curr_pos];
}


