/*

  term.c
  
*/

#include "term.h"

void term_Init(term_t *term, uint8_t width, uint8_t height, char *memory, term_show_cb show)
{
  term->width = width;
  term->height = height;
  term->screen = memory;
  term->show = show;
  term_Clear(term);
}

void term_Home(term_t *term)
{
  term->cursor_x = 0;
  term->cursor_y = 0;
}

void term_Clear(term_t *term)
{
  uint16_t cnt;
  char *ptr;
  cnt = term->width;
  cnt *= term->height;
  ptr = term->screen;
  while( cnt > 0 )
  {
    *ptr++ = ' ';
    cnt--;
  }
  term_Home(term);
}

static void term_ScrollScreenUp(term_t *term)
{
  uint8_t y, cnt;
  char *dest;
  char *src;
  
  /* move content one line up */
  for( y = 1; y < term->height; y++ )
  {
    cnt = term->width;
    dest = term->screen;
    dest += (y-1)*cnt;
    src = dest;
    src += cnt;
    do
    {
      *dest++ = *src++;
      cnt--;
    } while( cnt > 0 );
  }
  
  /* fill lowest line with blank */
  cnt = term->width;
  y = term->height;
  y--;
  dest = term->screen;
  dest += y*cnt;
  do
  {
    *dest++ = ' ';
    cnt--;
  } while( cnt > 0 );
}



void term_NewLine(term_t *term)
{
  if ( term->cursor_y < term->height )
  {
    term->show(term, TERM_MSG_PRE_NEWLINE);
    term->cursor_y++;
    term->cursor_x = 0;
    term->show(term, TERM_MSG_POST_NEWLINE);
  }
  else
  {
    /* handle pending scroll up */
    term->show(term, TERM_MSG_PRE_SCROLL);
    term_ScrollScreenUp(term);
    term->cursor_x = 0;
    term->show(term, TERM_MSG_POST_SCROLL);
  }
  
}

void term_WriteChar(term_t *term, char c)
{
  /* handle special chars */
  if ( c == '\n' )
  {
    term_NewLine(term);
    return;
  }
  
  if ( c < 32 )
    return;
  
  /* handle pending scroll up */
  if ( term->cursor_y >= term->height )
  {
    term->show(term, TERM_MSG_PRE_SCROLL);
    term_ScrollScreenUp(term);
    term->cursor_y = term->height - 1;
    term->cursor_x = 0;
    term->show(term, TERM_MSG_POST_SCROLL);
  }
  
  /* write char */
  term->show(term, TERM_MSG_PRE_CHAR);
  term->screen[term->cursor_y * term->width + term->cursor_x] = c;
  term->show(term, TERM_MSG_POST_CHAR);
  
  /* update cursor x pos */
  term->cursor_x++;
  if ( term->cursor_x >= term->width )
  {
    term_NewLine(term);
  }  
}

void term_WriteString(term_t *term, const char *str)
{
  while( *str != '\0' )
  {
    term_WriteChar(term, *str++);
  }
}


#ifdef TEST

#include <stdio.h>

void show(term_t *term)
{
  uint16_t x, y;
  for( y = 0; y < term->height; y++ )
  {
    printf("%02d |", y);
    for( x = 0; x < term->width; x++ )
    {
      printf("%c", term->screen[y * term->width + x]);
    }
    printf("|\n");
  }
  printf("%d %d\n", term->cursor_x, term->cursor_y);
}

void show_cb(term_t *term, uint8_t msg)
{
  switch(msg)
  {
    case TERM_MSG_POST_NEWLINE:
    case TERM_MSG_POST_SCROLL:
    case TERM_MSG_POST_CHAR:
      show(term);
      break;
    
  }
}

void main(void)
{
  term_t t;
  char buf[10*4];
  term_Init(&t, 10, 4, buf, show_cb);
  term_WriteString(&t, "12345");
  term_WriteString(&t, "abcdefg");
  term_WriteString(&t, "\n");
  term_WriteString(&t, "abc\n");
  term_WriteString(&t, "12345678901234");
  term_WriteString(&t, "\n");
  term_WriteString(&t, "a");
}

#endif

