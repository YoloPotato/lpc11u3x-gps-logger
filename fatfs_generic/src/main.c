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


/*=======================================================================*/
/* u8x8_lpc11u3x.c */
uint8_t u8x8_gpio_and_delay_lpc11u3x(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/*=======================================================================*/
/* little_rook_chess.c */
void chess_Init(u8g2_t *u8g, uint8_t body_color);
void chess_exec(void) ;


/*=======================================================================*/
u8x8_t u8x8;

FATFS FatFs;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */


/*=======================================================================*/
/* system procedures and sys tick master task */

#define SYS_TICK_PERIOD_IN_MS 100


volatile uint32_t sys_tick_irq_cnt=0;


void __attribute__ ((interrupt)) SysTick_Handler(void)
{
  sys_tick_irq_cnt++;
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

  //delay_micro_seconds(10UL*1000UL);

  u8x8_Setup(&u8x8, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_i2c, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_lpc11u3x);

  //u8g2_Setup_ssd1306_i2c_128x64_noname_1(&u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_lpc11u3x);
  u8x8_InitDisplay(&u8x8);
  u8x8_ClearDisplay(&u8x8);
  u8x8_SetPowerSave(&u8x8, 0);
  u8x8_SetFont(&u8x8, u8x8_font_amstrad_cpc_extended_r);
  u8x8_DrawString(&u8x8, 0, y++, "LPC11U35");

  if ( EEPROM_Test() != 0 )
  {
    u8x8_DrawString(&u8x8, 0, y++, "EEPROM ok");    
  }
  else
  {
    u8x8_DrawString(&u8x8, 0, y++, "EEPROM failed");    
  }


  fr = f_mount(&FatFs, "", 1);		/* Give a work area to the default drive and force a mount (http://elm-chan.org/fsw/ff/en/mount.html) */
  if ( fr == FR_OK )
  {
    char buf[24];
    u8x8_DrawString(&u8x8, 0, y, "Mount:");    
    f_getlabel("", buf, NULL);
    u8x8_DrawString(&u8x8, 6, y, buf);    
    y++;

    fr = f_open(&Fil, "newfile.txt", FA_WRITE | FA_CREATE_ALWAYS); /* Create a file */
    if ( fr == FR_OK) 
    {	
      UINT bw;
      u8x8_DrawString(&u8x8, 0, y++, "File open ok");    
      fr =f_write(&Fil, "It works!\r\n", 11, &bw);	/* Write data to the file */
      f_close(&Fil);								/* Close the file */

    }
    
  }
  else
  {
    u8x8_DrawString(&u8x8, 0, y++, "Mount failed");    
    u8x8_DrawString(&u8x8, 0, y++, fr_to_str[fr]);    
    
  }
  


  for(;;)
  {
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

