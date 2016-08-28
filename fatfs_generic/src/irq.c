/*

  irq.c
  
  IRQ enable and restore procedures
  
*/


#include "chip.h"

uint32_t disable_irq(void) 
{
  uint32_t state;
  state = __get_PRIMASK();
  __disable_irq();
  return state;
}
 
void restore_irq(uint32_t state)
{
  __set_PRIMASK(state);
}