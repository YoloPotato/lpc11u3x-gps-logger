
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

/* Setup system clocking */
static void SystemSetupClocking(void)
{
	volatile int i;

	/* Powerup main oscillator */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSOSC_PD);

	/* Wait 200us for OSC to be stabilized, no status
	   indication, dummy wait. */
	for (i = 0; i < 0x100; i++) {}
	Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_IRC);
	/* Set system PLL input to main oscillator */
	Chip_Clock_SetSystemPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);

	/* Power down PLL to change the PLL divider ratio */
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Setup PLL for main oscillator rate (FCLKIN = 12MHz) * 4 = 48MHz
	   MSEL = 3 (this is pre-decremented), PSEL = 1 (for P = 2)
	   FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 4 = 48MHz
	   FCCO = FCLKOUT * 2 * P = 48MHz * 2 * 2 = 192MHz (within FCCO range) */
	Chip_Clock_SetupSystemPLL(3, 1);

	/* Powerup system PLL */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Wait for PLL to lock */
	while (!Chip_Clock_IsSystemPLLLocked()) {}

	/* Set system clock divider to 1 */
	Chip_Clock_SetSysClockDiv(1);

	/* Setup FLASH access to 3 clocks */
	Chip_FMC_SetFLASHAccess(FLASHTIM_50MHZ_CPU);

	/* Set main clock source to the system PLL. This will drive 48MHz
	   for the main clock and 48MHz for the system clock */
	Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLOUT);

	/* Set USB PLL input to main oscillator */
	Chip_Clock_SetUSBPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);
	/* Setup USB PLL  (FCLKIN = 12MHz) * 4 = 48MHz
	   MSEL = 3 (this is pre-decremented), PSEL = 1 (for P = 2)
	   FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 4 = 48MHz
	   FCCO = FCLKOUT * 2 * P = 48MHz * 2 * 2 = 192MHz (within FCCO range) */
	Chip_Clock_SetupUSBPLL(3, 1);

	/* Powerup USB PLL */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPLL_PD);

	/* Wait for PLL to lock */
	while (!Chip_Clock_IsUSBPLLLocked()) {}
}


USBD_HANDLE_T g_hUsb;
const USBD_API_T *g_pUsbApi;	// according to usbd_rom_api.h, the variable name has to be g_pUsbApi, which is defines as USBD_API
USBD_API_INIT_PARAM_T usb_param;
USB_CORE_DESCS_T desc;


static ErrorCode_t update_device_status_patch(USBD_HANDLE_T hUsb)
{
	USB_CORE_CTRL_T *pCtrl;          
	pCtrl = (USB_CORE_CTRL_T *) hUsb;      /* convert the handle to control structure */
	if ( (((USB_CONFIGURATION_DESCRIPTOR *) (pCtrl->full_speed_desc))->bmAttributes & USB_CONFIG_POWERED_MASK) != 0 ) {
		/* This is SELF_POWERED. */
		pCtrl->device_status |= (0x01 << 0);
	}
	else {
		/* This is BUS_POWERED. */
		pCtrl->device_status &= ~(0x01 << 0);                     
	}
	if ( (((USB_CONFIGURATION_DESCRIPTOR *) (pCtrl->full_speed_desc))->bmAttributes & USB_CONFIG_REMOTE_WAKEUP) != 0 ) {
		/* This is REMOTE_WAKEUP enabled. */
		pCtrl->device_status |= (0x01 << 1);
	}
	else {
		/* This is REMOTE_WAKEUP disabled. */
		pCtrl->device_status &= ~(0x01 << 1);
	}
	return LPC_OK;
}


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

/* Handler for WCID USB device requests. */
static ErrorCode_t WCID_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	USB_CORE_CTRL_T *pCtrl = (USB_CORE_CTRL_T *) hUsb;
	ErrorCode_t ret = ERR_USBD_UNHANDLED;

	/* Handle Microsoft's WCID request for install less WinUSB operation.
	   Check https://github.com/pbatard/libwdi/wiki/WCID-Devices for more details.
	 */
	if (event == USB_EVT_SETUP) {
		switch (pCtrl->SetupPacket.bmRequestType.BM.Type) {
		case REQUEST_STANDARD:
			if ((pCtrl->SetupPacket.bmRequestType.BM.Recipient == REQUEST_TO_DEVICE) &&
				(pCtrl->SetupPacket.bRequest == USB_REQUEST_GET_DESCRIPTOR) &&
				(pCtrl->SetupPacket.wValue.WB.H == USB_BOS_TYPE)) {
				pCtrl->EP0Data.pData = (uint8_t *) USB_BOSDescriptor;
				pCtrl->EP0Data.Count = MIN(USB_BOSDescriptorSize, pCtrl->SetupPacket.wLength);
				USBD_API->core->DataInStage(pCtrl);
				ret = LPC_OK;
			}
            else if ((pCtrl->SetupPacket.bmRequestType.BM.Recipient == REQUEST_TO_DEVICE) &&
                     (pCtrl->SetupPacket.bRequest == USB_REQUEST_GET_DESCRIPTOR) &&
                     (pCtrl->SetupPacket.wValue.WB.H == USB_STRING_DESCRIPTOR_TYPE) &&
                     (pCtrl->SetupPacket.wValue.WB.L == 0x00EE)) {
                pCtrl->EP0Data.pData = (uint8_t *) WCID_String_Descriptor;
                pCtrl->EP0Data.Count = MIN(WCID_String_DescriptorSize, pCtrl->SetupPacket.wLength);
                USBD_API->core->DataInStage(pCtrl);
                ret = LPC_OK;
            }
			break;
        case REQUEST_VENDOR:
            if (pCtrl->SetupPacket.bRequest != WCID_VENDOR_CODE) {
                break;
            }
            switch (pCtrl->SetupPacket.bmRequestType.BM.Recipient) {
                case REQUEST_TO_DEVICE:
                    if (pCtrl->SetupPacket.wIndex.W == 0x0004) {
                        pCtrl->EP0Data.pData = (uint8_t *) WCID_CompatID_Descriptor;
                        pCtrl->EP0Data.Count = MIN(WCID_CompatID_DescriptorSize, pCtrl->SetupPacket.wLength);
                        USBD_API->core->DataInStage(pCtrl);
                        ret = LPC_OK;
                    }
                    /* Fall-through. Check note1 of
                       https://github.com/pbatard/libwdi/wiki/WCID-Devices#wiki-Defining_a_Device_Interface_GUID_or_other_device_specific_properties
                       break;
                    */

                case REQUEST_TO_INTERFACE:
                    if (pCtrl->SetupPacket.wIndex.W == 0x0005) {
                        pCtrl->EP0Data.pData = (uint8_t *) WCID_ExtProp_Descriptor;
                        pCtrl->EP0Data.Count = MIN(WCID_ExtProp_DescriptorSize, pCtrl->SetupPacket.wLength);
                        USBD_API->core->DataInStage(pCtrl);
                        ret = LPC_OK;
                    }
                    break;
            }
            break;
        }
	}

	return ret;
}



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

	/* Init USB API structure */
	g_pUsbApi = (const USBD_API_T *) LPC_ROM_API->usbdApiBase;

	/* initialize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;
	/*	WORKAROUND for artf44835 ROM driver BUG:
	    Code clearing STALL bits in endpoint reset routine corrupts memory area
	    next to the endpoint control data. For example When EP0, EP1_IN, EP1_OUT,
	    EP2_IN are used we need to specify 3 here. But as a workaround for this
	    issue specify 4. So that extra EPs control structure acts as padding buffer
	    to avoid data corruption. Corruption of padding memory doesn�t affect the
	    stack/program behavior.
	 */
	usb_param.max_num_ep = 1 + 1;
	usb_param.mem_base = USB_STACK_MEM_BASE;
	usb_param.mem_size = USB_STACK_MEM_SIZE;
	usb_param.USB_Reset_Event = update_device_status_patch;
	// usb_param.USB_Configure_Event = USB_Configure_Event;

	/* Set the USB descriptors */
	desc.device_desc = (uint8_t *) &USB_DeviceDescriptor;
	desc.string_desc = (uint8_t *) &USB_StringDescriptor[0];
	/* Note, to pass USBCV test full-speed only devices should have both
	 * descriptor arrays point to same location and device_qualifier set
	 * to 0.
	 */
	desc.high_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.full_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.device_qualifier = 0;

	/* USB Initialization */
	ret = USBD_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK) {
		
		/* Link Power Management is supported. */
    LPC_USB->DEVCMDSTAT |= (0x1<<11);
    LPC_USB->LPM |= (0x2<<4);/* RESUME duration. */

		/*	WORKAROUND for artf32219 ROM driver BUG:
		    The mem_base parameter part of USB_param structure returned
		    by Init() routine is not accurate causing memory allocation issues for
		    further components.
		 */
		usb_param.mem_base = USB_STACK_MEM_BASE + (USB_STACK_MEM_SIZE - usb_param.mem_size);
		
		/* register WCID handler */
		ret = USBD_API->core->RegisterClassHandler(g_hUsb, WCID_hdlr, 0);
		
		ret = usb_dfu_init(g_hUsb,
						   (USB_INTERFACE_DESCRIPTOR *) &USB_FsConfigDescriptor[sizeof(USB_CONFIGURATION_DESCRIPTOR)],
						   &usb_param.mem_base, &usb_param.mem_size);
		
		if (ret == LPC_OK) {
				/* enable USB interrupts */
	NVIC_EnableIRQ(USB0_IRQn);
			/* now connect */
			USBD_API->hw->Connect(g_hUsb, 1);
		}
	}

#ifdef XYZ
	while (1) {
				if (dfu_detach_sig)
				{
				  uint16_t i;
					for (i = 0; i < 0x4000; i++) {
						/* wait before detach */
					}
					enter_DFU_SL(g_hUsb);
				}
				else {
					__WFI();
				}
			
		}
#endif
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
  //Chip_SetupXtalClocking();	// this does NOT activate the USB Clock
  SystemSetupClocking();	// local copy --> will also activate USB Clock

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
  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 19);	/* port 1, pin 19: Disable Battery Pack Load */
  Chip_GPIO_SetPinOutLow(LPC_GPIO, 1, 19);
  
  usb_init();

  for(;;)
  {
    uint32_t t;
    Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 7);
    delay_micro_seconds(500UL*1000UL);
    
    Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 7);    
    delay_micro_seconds(500UL*1000UL);
    
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 19);	/* port 1, pin 19: Short Circuit*/
    
    Chip_GPIO_SetPinOutHigh(LPC_GPIO, 1, 19);
    delay_micro_seconds(400UL*1000UL);
    Chip_GPIO_SetPinOutLow(LPC_GPIO, 1, 19);
    
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

