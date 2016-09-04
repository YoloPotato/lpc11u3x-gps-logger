/*

  display.c 
  
*/

#include "display.h"
#include "term.h"
#include "u8g2.h"


/*=======================================================================*/
/* u8x8_lpc11u3x.c */
uint8_t u8x8_gpio_and_delay_lpc11u3x(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);


/*=======================================================================*/

#define WIDTH 16
#define HEIGHT 8

/*=======================================================================*/

term_t display_term;
u8x8_t display_u8x8;
char display_buffer[WIDTH*HEIGHT];

/*=======================================================================*/

static void display_write_screen(term_t *term)
{
  uint8_t x, y;
  char *ptr = term->screen;
  for( y = 0; y < term->height; y++ )
  {
    for( x = 0; x < term->width; x++ )
    {
      u8x8_DrawGlyph(&display_u8x8, x, y, *ptr);
      ptr++;
    }
  }
}

void display_show_cb(term_t *term, uint8_t msg)
{
  switch(msg)
  {
    case TERM_MSG_POST_NEWLINE:
    case TERM_MSG_POST_SCROLL:
    case TERM_MSG_POST_CHAR:
      display_write_screen(term);
      break;
    
  }
}

void display_Init(void)
{
  u8x8_Setup(&display_u8x8, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_i2c, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_lpc11u3x);

  //u8g2_Setup_ssd1306_i2c_128x64_noname_1(&u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_lpc11u3x);
  u8x8_InitDisplay(&display_u8x8);
  u8x8_ClearDisplay(&display_u8x8);
  u8x8_SetPowerSave(&display_u8x8, 0);
  u8x8_SetFont(&display_u8x8, u8x8_font_amstrad_cpc_extended_r);  
  
  term_Init(&display_term, WIDTH, HEIGHT, display_buffer, display_show_cb);
}

void display_Write(const char *str)
{
  term_WriteString(&display_term, str);
}

