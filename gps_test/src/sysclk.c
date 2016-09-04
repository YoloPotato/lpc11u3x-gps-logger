/*
  sysclk.c
*/

#include <chip.h>

/*
  According to chip.h there must be two variables which have to be defined
  by a user program: OscRateIn and ExtRateIn
  Values can be 0, but if not 0, then these values define the clock speed
  Actually there is one more variable (SystemCoreClock) which is
  updated by SystemCoreClockUpdate() based on the values given in the
  other two values.
*/
const uint32_t OscRateIn = 12000000;	/* freq. of the external crystal osc */
const uint32_t ExtRateIn = 0;


