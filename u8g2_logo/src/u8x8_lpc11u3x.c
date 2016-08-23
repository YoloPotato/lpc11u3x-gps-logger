/*
  u8x8_lpc11u3x.c
*/

#include "chip.h"
#include "delay.h"
#include "u8x8.h"

#define I2C_CLOCK_PIN 6
#define I2C_CLOCK_PORT 0
#define I2C_DATA_PIN  21
#define I2C_DATA_PORT 0

uint8_t u8x8_gpio_and_delay_lpc11u3x(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      /* only support for software I2C*/
      Chip_IOCON_PinMuxSet(LPC_IOCON, I2C_CLOCK_PORT, I2C_CLOCK_PIN, IOCON_MODE_PULLUP);
      Chip_IOCON_PinMuxSet(LPC_IOCON, I2C_DATA_PORT, I2C_DATA_PIN, IOCON_MODE_PULLUP);
      Chip_GPIO_SetPinDIRInput(LPC_GPIO, I2C_CLOCK_PORT, I2C_CLOCK_PIN);
      Chip_GPIO_SetPinDIRInput(LPC_GPIO, I2C_DATA_PORT, I2C_DATA_PIN);    
      break;
    case U8X8_MSG_DELAY_NANO:
      /* not required for SW I2C */
      break;
    
    case U8X8_MSG_DELAY_10MICRO:
      /* not used at the moment */
      break;
    
    case U8X8_MSG_DELAY_100NANO:
      /* not used at the moment */
      break;
   
    case U8X8_MSG_DELAY_MILLI:
      delay_micro_seconds(arg_int*1000UL);
      break;
    case U8X8_MSG_DELAY_I2C:
      /* arg_int is 1 or 4: 100KHz (5us) or 400KHz (1.25us) */
      delay_micro_seconds(arg_int<=2?5:1);
      break;
    
    case U8X8_MSG_GPIO_I2C_CLOCK:
      if ( arg_int == 0 )
      {
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, I2C_CLOCK_PORT, I2C_CLOCK_PIN);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, I2C_CLOCK_PORT, I2C_CLOCK_PIN);
      }
      else
      {
	//Chip_GPIO_SetPinOutHigh(LPC_GPIO, I2C_CLOCK_PORT, I2C_CLOCK_PIN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, I2C_CLOCK_PORT, I2C_CLOCK_PIN);
      }
      break;
    case U8X8_MSG_GPIO_I2C_DATA:
      if ( arg_int == 0 )
      {
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, I2C_DATA_PORT, I2C_DATA_PIN);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, I2C_DATA_PORT, I2C_DATA_PIN);
      }
      else
      {
	//Chip_GPIO_SetPinOutHigh(LPC_GPIO, I2C_DATA_PORT, I2C_DATA_PIN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, I2C_DATA_PORT, I2C_DATA_PIN);
      }
      break;
    case U8X8_PIN_MENU_SELECT:
      u8x8_SetGPIOResult(u8x8, 1);
      break;
    case U8X8_PIN_MENU_NEXT:
      u8x8_SetGPIOResult(u8x8, 1);
      break;
    case U8X8_PIN_MENU_PREV:
      u8x8_SetGPIOResult(u8x8, 1);
      break;
    case U8X8_PIN_MENU_HOME:
      u8x8_SetGPIOResult(u8x8, 1);
      break;
  }
  return 1;
}
