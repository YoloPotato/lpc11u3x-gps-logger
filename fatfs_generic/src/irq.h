/*

  irq.h
  
  IRQ enable and restore procedures
  
*/

#include <stdint.h>

#ifndef _IRQ_H
#define _IRQ_H

uint32_t disable_irq(void);
void restore_irq(uint32_t state);

#endif