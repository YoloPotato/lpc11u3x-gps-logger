
/*

  startup.c

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


  Requires LPC11U3x Chip Library
  The chip library can be downloaded from LPCOpen
  and is included in the file
  lpcopen_v2_03_lpcxpresso_nxp_lpcxpresso_11u37h.zip

*/
#include <chip.h>
#include "delay.h"


/*=======================================================================*/
/* external functions */
void __attribute__ ((interrupt)) SysTick_Handler(void);
void __attribute__ ((interrupt, used)) UART_IRQ(void);
int __attribute__ ((noinline)) main(void);



/*=======================================================================*/
/* 
  Reserve some space for the stack. This is used to check if global variables + stack exceed RAM size.
  If -Wl,--gc-sections is used, then also define -Wl,--undefined=arm_stack_area to keep the variable in RAM.
  The name of the variable (here: arm_stack_area) does not matter.

  Heap (=dynamic memory allocation) is not supported
*/
#ifndef __STACK_SIZE
#define __STACK_SIZE 0x100
#endif
unsigned char arm_stack_area[__STACK_SIZE] __attribute__ ((section(".stack"))) __attribute__ ((aligned(8)));


/*=======================================================================*/
/* isr system procedures */

/* make the top of the stack known to the c compiler, value will be calculated by the linker script */
void __StackTop(void);

void __attribute__ ((interrupt)) __attribute__ ((noreturn)) Reset_Handler(void)
{
  register unsigned long *ptr;
  register unsigned long *start;
  register unsigned long *end;

  /*     
  Loop to copy data from read only memory to RAM. The ranges
  of copy from/to are specified by following symbols evaluated in 
  linker script.
  __etext: End of code section, i.e., begin of data sections to copy from.
  __data_start__/__data_end__: RAM address range that data should be
  copied to. Both must be aligned to 4 bytes boundary.  
  */
  extern unsigned long __data_start__[];
  extern unsigned long __data_end__[];
  extern unsigned long __etext[];
  ptr = __etext;
  start = __data_start__;
  end = __data_end__;
  while( start < end )
  {
    *start = *ptr;
    start++;
    ptr++;
  }
  
  /*
  Loop to zero out BSS section, which uses following symbols
  in linker script:
  __bss_start__: start of BSS section. Must align to 4
  __bss_end__: end of BSS section. Must align to 4
  */
  extern unsigned long __bss_start__[];
  extern unsigned long __bss_end__[];
  ptr = __bss_start__;
  end = __bss_end__;
  while( ptr < end )
  {
    *ptr = 0;
    ptr++;
  }

  /* Call main procedure */  
  main();
  
  /* finished, do nothing. */
  for(;;)
    ;
}

/* "NMI_Handler" is used in the ld script to calculate the checksum */
void __attribute__ ((interrupt)) NMI_Handler(void)
{
}

/* "HardFault_Handler" is used in the ld script to calculate the checksum */
void __attribute__ ((interrupt)) HardFault_Handler(void)
{
}

/* make the checksum known to the c compiler, value will be calculated by the linker script */
void LPC_checksum(void);

/*=======================================================================*/
/* isr vector */
/* see page 445 of the LPC11U3x user manual */ 
/* see also enum LPC11UXX_IRQn in isr/cmsis_11uxx.h */

typedef void (*isr_handler_t)(void);
isr_handler_t __isr_vector[48] __attribute__ ((section(".isr_vector"))) __attribute__ ((aligned(4)))= 
{
  __StackTop,			/* 0x00: Top of Stack, calculated by the linker script */
  Reset_Handler,		/* 0x04: Reset Handler, DO NOT CHANGE THE ISR NAME (used for LPC_checksum calculation) */
  NMI_Handler,			/* 0x08: NMI Handler, DO NOT CHANGE THE ISR NAME (used for LPC_checksum calculation) */
  HardFault_Handler,         /* 0x0c: Hard Fault Handler, DO NOT CHANGE THE ISR NAME (used for LPC_checksum calculation) */
  0,					/* 0x10: Reserved, must be 0 */
  0,					/* 0x14: Reserved, must be 0 */
  0,					/* 0x18: Reserved, must be 0 */
  LPC_checksum,		/* 0x1c: Checksum, calculated by the linker script or the flash utility */
  0,                                /* Reserved */
  0,                                /* Reserved */
  0,                                /* Reserved */
  0,                                /* SVCall Handler */
  0,                                /* Reserved */
  0,                                /* Reserved */
  0,                                /* PendSV Handler */
  SysTick_Handler,         /* SysTick Handler */            
  
  0,                                /* PIN_INT0_IRQn                 = 0,		 Pin Interrupt 0 */
  0,                                /* PIN_INT1_IRQn                 = 1,		 Pin Interrupt 1 */
  0,                                /* PIN_INT2_IRQn                 = 2,		 Pin Interrupt 2 */
  0,                                /* PIN_INT3_IRQn                 = 3,		 Pin Interrupt 3 */
  0,                                /* PIN_INT4_IRQn                 = 4,		 Pin Interrupt 4 */
  0,                                /* PIN_INT5_IRQn                 = 5,		 Pin Interrupt 5 */
  0,                                /* PIN_INT6_IRQn                 = 6,		 Pin Interrupt 6 */
  0,                                /* PIN_INT7_IRQn                 = 7,		Pin Interrupt 7 */
  0,                                /* GINT0_IRQn                     = 8,		GPIO GROUP 0 interrupt */
  0,                                /* GINT1_IRQn                    = 9,			GPIO GROUP 1 interrupt */
  0,                                /* Reserved10_IRQn               = 10,		Reserved Interrupt */
  0,                                /* Reserved11_IRQn               = 11, */
  0,                                /* Reserved12_IRQn               = 12, */
  0,                                /* Reserved13_IRQn               = 13, */
  0,                                /* SSP1_IRQn                     = 14,		SSP1 Interrupt */
  0,                                /* I2C0_IRQn                     = 15,		 	I2C Interrupt */
  0,                                /* TIMER_16_0_IRQn               = 16,		 16-bit Timer0 Interrupt                          */
  0,                                /* TIMER_16_1_IRQn               = 17,		 16-bit Timer1 Interrupt                          */
  0,                                /* TIMER_32_0_IRQn               = 18,		 32-bit Timer0 Interrupt                          */
  0,                                /* TIMER_32_1_IRQn               = 19,		 32-bit Timer1 Interrupt                          */
  0,                                /* SSP0_IRQn                     = 20,		 SSP0 Interrupt                                   */
  UART_IRQ,                    /* UART0_IRQn                    = 21,		 UART Interrupt                                   */
  0,                                /* USB0_IRQn                     = 22,		 USB IRQ interrupt                                */
  0,                                /* USB0_FIQ_IRQn                 = 23,		 USB FIQ interrupt                                */
  0,                                /* ADC_IRQn                      = 24,		 A/D Converter Interrupt                          */
  0,                                /* WDT_IRQn                      = 25,		 Watchdog timer Interrupt                         */
  0,                                /* BOD_IRQn                      = 26,		 Brown Out Detect(BOD) Interrupt                  */
  0,                                /* FMC_IRQn                      = 27,		 Flash Memory Controller Interrupt                */
  0,                                /* RESERVED28_IRQn               = 28, */
  0,                                /* RESERVED29_IRQn               = 29, */
  0,                                /* USB_WAKEUP_IRQn               = 30,		 USB wake-up interrupt Interrupt                  */
  0                                /* IOH_IRQn                      = 31,		 I/O Handler IRQ (Only for LPC11U37)              */
};

