
/*
  Include LPC11U3x Chip Library
  The chip library can be downloaded from LPCOpen
  and is included in the file
  lpcopen_v2_03_lpcxpresso_nxp_lpcxpresso_11u37h.zip
*/
#include <chip.h>
#include <usbd_rom_api.h>
#include <string.h>
#include "delay.h"

#define SYS_TICK_PERIOD_IN_MS 100


/*=======================================================================*/
/* system procedures and sys tick master task */

volatile uint32_t sys_tick_irq_cnt=0;


void __attribute__ ((interrupt)) SysTick_Handler(void)
{
  sys_tick_irq_cnt++;
}

USBD_HANDLE_T g_hUsb;
const USBD_API_T *g_pUsbApi;	// according to usbd_rom_api.h, the variable name has to be g_pUsbApi, which is defines as USBD_API
USBD_API_INIT_PARAM_T usb_param;
USB_CORE_DESCS_T usb_desc;

/* Handle interrupt from USB */
void __attribute__ ((interrupt)) USB_Handler(void)
{

	uint32_t *addr = (uint32_t *) LPC_USB->EPLISTSTART;
	/*	WORKAROUND for artf32289 ROM driver BUG:
	    As part of USB specification the device should respond
	    with STALL condition for any unsupported setup packet. The host will send
	    new setup packet/request on seeing STALL condition for EP0 instead of sending
	    a clear STALL request. Current driver in ROM doesn't clear the STALL
	    condition on new setup packet which should be fixed.
	 */
	if ( LPC_USB->DEVCMDSTAT & _BIT(8) ) {	/* if setup packet is received */
		addr[0] &= ~(_BIT(29));	/* clear EP0_OUT stall */
		addr[2] &= ~(_BIT(29));	/* clear EP0_IN stall */
	}
	USBD_API->hw->ISR(g_hUsb);
}

/*
  this is how the LPC11U35 reports itself, when booted in flash mode:
    New USB device found, idVendor=1fc9, idProduct=000f
    New USB device strings: Mfr=1, Product=2, SerialNumber=3
    Product: LPC1XXX IFLASH
    Manufacturer: NXP
    SerialNumber: ISP
*/
USB_DEVICE_DESCRIPTOR USB_DeviceDescriptor = {
	USB_DEVICE_DESC_SIZE,				/* bLength */
	USB_DEVICE_DESCRIPTOR_TYPE,			/* bDescriptorType */
	0x0200,						/* bcdUSB: 2.00 */
	0xEF,								/* bDeviceClass */
	0x02,								/* bDeviceSubClass */
	0x01,								/* bDeviceProtocol */
	USB_MAX_PACKET0,					/* bMaxPacketSize0 */
	0x1FC9,						/* idVendor */
	0x000f,						/* idProduct, use the same as tie boot loader, does this make sens??? */
	0x0100,						/* bcdDevice: 1.00 */
	0x01,								/* iManufacturer */
	0x02,								/* iProduct */
	0x03,								/* iSerialNumber */
	0x01								/* bNumConfigurations: This defines the number of ConfigDescriptors */
};

#ifdef xyz
USB_DEVICE_DESCRIPTOR USB_DeviceDescriptor = {
	USB_DEVICE_DESC_SIZE,			/* bLength */
	USB_DEVICE_DESCRIPTOR_TYPE,		/* bDescriptorType */
	0x0201,	/* 2.00 */          /* bcdUSB */
	0x00,							/* bDeviceClass */
	0x00,							/* bDeviceSubClass */
	0x00,							/* bDeviceProtocol */
	USB_MAX_PACKET0,				/* bMaxPacketSize0 */
	0x1FC9,					/* idVendor */
	0x5002,					/* idProduct */
	0x0100,	        /* 1.00 */          /* bcdDevice */
	0x01,							/* iManufacturer */
	0x02,							/* iProduct */
	0x03,							/* iSerialNumber */
	0x01							/* bNumConfigurations */
};
#endif

void usb_init(void)
{
  ErrorCode_t ret = LPC_OK;
  
  /* enable USB main clock */
  Chip_Clock_SetUSBClockSource(SYSCTL_USBCLKSRC_PLLOUT, 1);
  /* Enable AHB clock to the USB block and USB RAM. */
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USB);
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USBRAM);
  /* power UP USB Phy */
  Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPAD_PD);

  // Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON); /* alread enabled */
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0,  3,  (IOCON_FUNC1 | IOCON_MODE_INACT));		/* PIO0_3 used for USB_VBUS */
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0,  6,  (IOCON_FUNC1 | IOCON_MODE_INACT));		/* PIO0_6 used for USB_CONNECT */

  g_pUsbApi = (const USBD_API_T *) LPC_ROM_API->usbdApiBase;

  memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
  usb_param.usb_reg_base = LPC_USB0_BASE;  
  usb_param.max_num_ep = 4;
  usb_param.mem_base = USB_STACK_MEM_BASE;
  usb_param.mem_size = USB_STACK_MEM_SIZE;  

  usb_desc.device_desc = (uint8_t*)&USB_DeviceDescriptor;
  //usb_desc.string_desc = (uint8_t *) USB_StringDescriptor;
  //usb_desc.high_speed_desc = USB_FsConfigDescriptor;
  //usb_desc.full_speed_desc = USB_FsConfigDescriptor;
  //usb_desc.device_qualifier = 0;
  
  //ret = USBD_API->hw->Init(&g_hUsb, &usb_desc, &usb_param);
}


/*
  setup the hardware and start interrupts.
  called by "Reset_Handler"
*/
int __attribute__ ((noinline)) main(void)
{

  /* call to the lpc lib setup procedure. This will set the IRC as clk src and main clk to 48 MHz */
  /* it will also enable IOCON, see sysinit_11xx.c */
  //Chip_SystemInit(); 
  Chip_SetupXtalClocking();

  /* if the clock or PLL has been changed, also update the global variable SystemCoreClock */
  /* see chip_11xx.c */
  SystemCoreClockUpdate();
  
  /* set systick and start systick interrupt */
  SysTick_Config(SystemCoreClock/1000UL*(unsigned long)SYS_TICK_PERIOD_IN_MS);
  
  
  /* turn on GPIO */
  Chip_GPIO_Init(LPC_GPIO);
  
  /* turn on IOCON... this is also done in Chip_SystemInit(), but not in Chip_SetupXtalClocking() */
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
  
  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);	/* port 0, pin 7: LED on eHaJo Breakout Board */
  
  usb_init();

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
  0,                                /* UART0_IRQn                    = 21,		 UART Interrupt                                   */
  USB_Handler,              /* USB0_IRQn                     = 22,		 USB IRQ interrupt                                */
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

