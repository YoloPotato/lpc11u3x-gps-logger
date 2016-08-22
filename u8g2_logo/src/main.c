/*
  main.c
  
  Two function must be implemented:
    void __attribute__ ((interrupt)) SysTick_Handler(void)
    int __attribute__ ((noinline)) main(void)
    
    
*/

#include <chip.h>
#include "delay.h"



/*=======================================================================*/
/* system procedures and sys tick master task */

#define SYS_TICK_PERIOD_IN_MS 100


volatile uint32_t sys_tick_irq_cnt=0;


void __attribute__ ((interrupt)) SysTick_Handler(void)
{
  sys_tick_irq_cnt++;
}



/*=======================================================================*/
/*
  setup the hardware and start interrupts.
  called by "Reset_Handler"
*/
int __attribute__ ((noinline)) main(void)
{

  /* call to the lpc lib setup procedure. This will set the IRC as clk src and main clk to 48 MHz */
  /* it will also enable IOCON, see sysinit_11xx.c */
  Chip_SystemInit(); 

  /* if the clock or PLL has been changed, also update the global variable SystemCoreClock */
  /* see chip_11xx.c */
  SystemCoreClockUpdate();
  
  /* set systick and start systick interrupt */
  SysTick_Config(SystemCoreClock/1000UL*(unsigned long)SYS_TICK_PERIOD_IN_MS);
  
  
  /* turn on GPIO */
  Chip_GPIO_Init(LPC_GPIO);
  
  /* turn on IOCON... this is also done in Chip_SystemInit() */
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
  
  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);	/* port 0, pin 7: LED on eHaJo Breakout Board */

  for(;;)
  {
    uint32_t t;
    Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 7);
    delay_micro_seconds(500UL*1000UL);
    
    Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 7);    
    delay_micro_seconds(500UL*1000UL);
  }
  
  
  
  /* enter sleep mode: Reduce from 1.4mA to 0.8mA with 12MHz */  
  while (1)
  {
    SCB->SCR |= (1UL << SCB_SCR_SLEEPONEXIT_Pos);		/* enter sleep mode after interrupt */ 
    Chip_PMU_SleepState(LPC_PMU);						/* enter sleep mode now */
  }
}

