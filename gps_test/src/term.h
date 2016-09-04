/*

  term.h
  
*/

#ifndef _TERM_H
#define _TERM_H

#include <stdint.h>

/*
  SCROLL msg also includes NEWLINE, this means NEWLINE and SCROLL will not appear at the same time
*/
#define TERM_MSG_PRE_CHAR 1
#define TERM_MSG_POST_CHAR 2
#define TERM_MSG_PRE_NEWLINE 3
#define TERM_MSG_POST_NEWLINE 4
#define TERM_MSG_PRE_SCROLL 5
#define TERM_MSG_POST_SCROLL 6


typedef struct _term_struct term_t;
typedef void (*term_show_cb)(term_t *term, uint8_t msg);

struct _term_struct
{
  uint8_t width;	/* width of the screen buffer */
  uint8_t height;	/* height of the screen buffer */
  uint8_t cursor_x;
  uint8_t cursor_y;
  char *screen;		/* pointer to width*height bytes */
  term_show_cb show;
};


void term_Init(term_t *term, uint8_t width, uint8_t height, char *memory, term_show_cb show);
void term_Home(term_t *term);
void term_Clear(term_t *term);
void term_NewLine(term_t *term);
void term_WriteString(term_t *term, const char *str);

#endif

