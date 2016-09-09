/*
  main.c
  
  Two function must be implemented:
    void __attribute__ ((interrupt)) SysTick_Handler(void)
    int __attribute__ ((noinline)) main(void)
    
    
*/

#include <chip.h>
#include "delay.h"
#include "u8g2.h"
#include "ff.h"		/* Declarations of FatFs API */
#include "eeprom.h"
#include "display.h"
#include "gpx.h"




/*=======================================================================*/

FATFS FatFs;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */


/*=======================================================================*/
/* system procedures and sys tick master task */

#define SYS_TICK_PERIOD_IN_MS 100


volatile uint32_t sys_tick_irq_cnt=0;
volatile uint32_t uart_irq_cnt=0;
volatile uint32_t uart_data_cnt=0;
volatile uint8_t uart_data;

void __attribute__ ((interrupt)) SysTick_Handler(void)
{
  sys_tick_irq_cnt++;
}


void __attribute__ ((interrupt, used)) UART_IRQ(void)
{
  /* Read the IIR register. This is required. If not read the itq will not be cleared */
  uint32_t iir = Chip_UART_ReadIntIDReg(LPC_USART);
  if ( (iir & 1) == 0 )
  {
    /* count the number of interrupts */
    uart_irq_cnt++;
    while ( ( Chip_UART_ReadLineStatus(LPC_USART) & 1 ) != 0 )
    {
      /* count bytes */
      uart_data_cnt++;
      /* read the byte from the FIFO Buffer */
      uart_data = Chip_UART_ReadByte(LPC_USART);
    }
  }
}




const char *fr_to_str[] = 
{
	"OK",				/* (0) Succeeded */
	"DISK_ERR",			/* (1) A hard error occurred in the low level disk I/O layer */
	"INT_ERR",				/* (2) Assertion failed */
	"NOT_READY",			/* (3) The physical drive cannot work */
	"NO_FILE",				/* (4) Could not find the file */
	"NO_PATH",				/* (5) Could not find the path */
	"INVALID_NAME",		/* (6) The path name format is invalid */
	"DENIED",				/* (7) Access denied due to prohibited access or directory full */
	"EXIST",				/* (8) Access denied due to prohibited access */
	"INVALID_OBJECT",		/* (9) The file/directory object is invalid */
	"WRITE_PROTECTED",		/* (10) The physical drive is write protected */
	"INVALID_DRIVE",		/* (11) The logical drive number is invalid */
	"NOT_ENABLED",			/* (12) The volume has no work area */
	"NO_FILESYSTEM",		/* (13) There is no valid FAT volume */
	"MKFS_ABORTED",		/* (14) The f_mkfs() aborted due to any problem */
	"TIMEOUT",				/* (15) Could not get a grant to access the volume within defined period */
	"LOCKED",				/* (16) The operation is rejected according to the file sharing policy */
	"NOT_ENOUGH_CORE",		/* (17) LFN working buffer could not be allocated */
	"TOO_MANY_FILES",	/* (18) Number of open files > _FS_LOCK */
	"INVALID_PARA"	/* (19) Given parameter is invalid */
};



/*=======================================================================*/
/*
  setup the hardware and start interrupts.
  called by "Reset_Handler"
*/
int __attribute__ ((noinline)) main(void)
{
  FRESULT fr;
  uint8_t y = 0;
  uint32_t baud;

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
  
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON | SYSCTL_CLOCK_UART0 | SYSCTL_CLOCK_GPIO);
  
  
  /* this is the earliest time we can access the display (if any...) */
  display_Init();
  display_Write("LPC11U35 GPSLOG\n");

  /* configure LED for the eHaJo board */
  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);	/* port 0, pin 7: LED on eHaJo Breakout Board */
  
  /* setup UART for GPS */
  display_Write("UART Init\n");
  
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, IOCON_FUNC1);	/* RxD */
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 19, IOCON_FUNC1);	/* TxD */
  
  Chip_UART_Init(LPC_USART);
  Chip_UART_DisableDivisorAccess(LPC_USART);			/* divisor access must be disabled for IRQ setup, but this is already done in init */
  Chip_UART_IntEnable(LPC_USART, UART_IER_RBRINT);	/* enable receive interrupt */
  NVIC_EnableIRQ(UART0_IRQn);

  /* setup Baud rate */
  baud = Chip_UART_SetBaudFDR(LPC_USART, 9600);	/* puuhh... this is a HUGE function */
  display_Write("UART ");
  display_WriteUnsigned(baud);
  display_Write(" Baud\n");
  

  if ( EEPROM_Test() != 0 )
  {
    display_Write("EEPROM ok\n");
  }
  else
  {
    display_Write("EEPROM failed\n");
  }


  fr = f_mount(&FatFs, "", 1);		/* Give a work area to the default drive and force a mount (http://elm-chan.org/fsw/ff/en/mount.html) */
  if ( fr == FR_OK )
  {
    char buf[24];
    display_Write("Mount:");
    f_getlabel("", buf, NULL);
    display_Write(buf);
    display_Write("\n");
    y++;

    fr = f_open(&Fil, "newfile.txt", FA_WRITE | FA_CREATE_ALWAYS); /* Create a file */
    if ( fr == FR_OK) 
    {	
      UINT bw;
      fr =f_write(&Fil, "It works!\r\n", 11, &bw);	/* Write data to the file */
      f_close(&Fil);								/* Close the file */

    }
    
  }
  else
  {
    display_Write("Mount failed\n");
    display_Write(fr_to_str[fr]);
    display_Write("\n");
    
  }
  
  {
    gps_pos_t p;
    p.latitude = 1.1111;
    p.longitude = 2.2222;
    if ( gpx_write(&p) == 0 )
    {
      display_Write("gps wr failed\n");
    }
    else
    {
      display_Write("gps wr ok\n");
    }
  }
  


  for(;;)
  {
    Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 7);
    delay_micro_seconds(500UL*1000UL);
    
    Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 7);    
    delay_micro_seconds(500UL*1000UL);

    display_Write("IRQs: ");
    display_WriteUnsigned(uart_irq_cnt);
    display_Write("\n");
    
    
  }
  
  
  /* enter sleep mode: Reduce from 1.4mA to 0.8mA with 12MHz */  
  while (1)
  {
    SCB->SCR |= (1UL << SCB_SCR_SLEEPONEXIT_Pos);		/* enter sleep mode after interrupt */ 
    Chip_PMU_SleepState(LPC_PMU);						/* enter sleep mode now */
  }
}

