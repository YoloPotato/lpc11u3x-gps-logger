/*
  main.c
  
  Two function must be implemented:
    void __attribute__ ((interrupt)) SysTick_Handler(void)
    int __attribute__ ((noinline)) main(void)
    
    
*/

#include <chip.h>
#include "delay.h"
#include "u8g2.h"
#include "eeprom.h"
#include "display.h"
#include "gpx.h"
#include "pq.h"

/*=======================================================================*/
/* Configuration */
#define SYS_TICK_PERIOD_IN_MS 100

#define GPS_BAUD 9600

/* 1 measure per 10 sec */
//const char gps_set_rate_cmd[] = 
//"\xB5\x62\x06\x08\x06\x00\x10\x27\x01\x00\x01\x00\x4D\xDD\xB5\x62\x06\x08\x00\x00\x0E\x30";

/* 1 measure per 3 sec */
const char gps_set_rate_cmd[] = 
"\xB5\x62\x06\x08\x06\x00\xB8\x0B\x01\x00\x01\x00\xD9\x41\xB5\x62\x06\x08\x00\x00\x0E\x30";

/* when to send the gps_set_rate cmd: After 2 seconds (20 ticks) */
const uint32_t gps_set_rate_tick_delay = 20;

/* when to save the trackpoint */
/* if this is slower than the measures per seconds, then nothing will be saved */
/* until new GPS pos is available */
#define TRACKPOINT_SAVE_PERIOD_SECONDS 5

/*=======================================================================*/

pq_t pq;

/*=======================================================================*/
/* system procedures and sys tick master task */



volatile uint32_t sys_tick_irq_cnt=0;
volatile uint32_t uart_irq_cnt=0;
volatile uint32_t uart_data_cnt=0;
volatile uint32_t trackpoint_save_cnt = TRACKPOINT_SAVE_PERIOD_SECONDS*1000/SYS_TICK_PERIOD_IN_MS;
volatile uint8_t is_send_gps_rate = 0;


volatile uint8_t is_output_last_unknown_msg = 0;
volatile uint8_t is_output_uart_data_cnt = 0;
volatile uint8_t is_output_sat_cnt = 0;
volatile uint8_t is_output_gps_quality = 0;
volatile uint8_t is_output_altitude = 0;
volatile uint8_t is_output_pos = 0;
volatile uint8_t is_output_time = 0;
volatile uint8_t is_output_date = 0;
volatile uint8_t is_output_digit_cnt = 0;
volatile uint8_t is_output_gprmc_per_second = 0;

volatile uint8_t is_trackpoint_save = 0;
volatile uint8_t is_sd_write_success = 0;


void set_led_status(uint8_t status)
{
  if ( status )
    Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 7);
  else
    Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 7);
}

void __attribute__ ((interrupt)) SysTick_Handler(void)
{
  if ( sys_tick_irq_cnt == gps_set_rate_tick_delay )
  {
    is_send_gps_rate = 1;
  }
  
  sys_tick_irq_cnt++;

  /* time is over, let us save the trackpoint */
  if ( trackpoint_save_cnt > 0 )
  {
    trackpoint_save_cnt--;
  }
  else
  {
    is_trackpoint_save = 1;
    trackpoint_save_cnt = TRACKPOINT_SAVE_PERIOD_SECONDS*1000/SYS_TICK_PERIOD_IN_MS;
  }
  
  /* flash with LED for a successful write */
  if ( is_sd_write_success > 0 )
  {
    set_led_status(1);
    is_sd_write_success = 0;
  }
  else
  {
    set_led_status(0);
  }
    
  
  if ( (sys_tick_irq_cnt & 0x0ff) == 0 )
    is_output_uart_data_cnt = 1;
    
  /*
  if ( (sys_tick_irq_cnt & 0x0ff) == 0x01f )
    is_output_last_unknown_msg = 1;  
  */
  
  if ( (sys_tick_irq_cnt & 0x0ff) == 0x02f )
    is_output_sat_cnt = 1;  

  if ( (sys_tick_irq_cnt & 0x0ff) == 0x03f )
    is_output_gps_quality = 1;  
  
  if ( (sys_tick_irq_cnt & 0x0ff) == 0x04f )
    is_output_altitude = 1;
  
  if ( (sys_tick_irq_cnt & 0x0ff) == 0x05f )
    is_output_pos  = 1;

  if ( (sys_tick_irq_cnt & 0x0ff) == 0x06f )
    is_output_time = 1;
 
  if ( (sys_tick_irq_cnt & 0x0ff) == 0x06f )
    is_output_date = 1;
  
  if ( (sys_tick_irq_cnt & 0x0ff) == 0x07f )
    is_output_digit_cnt = 1;

  if ( (sys_tick_irq_cnt & 0x0ff) == 0x08f )
    is_output_gprmc_per_second = 1;

    
  
}


void __attribute__ ((interrupt, used)) UART_IRQ(void)
{  
  /* Read the IIR register. This is required. If not read the itq will not be cleared */
  uint32_t iir = Chip_UART_ReadIntIDReg(LPC_USART);
  uint8_t uart_data;
  
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
      pq_AddChar(&pq, uart_data);
    }
  }
}



/*=======================================================================*/
/*
  setup the hardware and start interrupts.
  called by "Reset_Handler"
*/
int __attribute__ ((noinline)) main(void)
{
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
  
  /* position parser init */
  pq_Init(&pq);
  
  /* setup UART for GPS */
  display_Write("UART Init\n");
  
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, IOCON_FUNC1);	/* RxD */
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 19, IOCON_FUNC1);	/* TxD */
  
  Chip_UART_Init(LPC_USART);
  Chip_UART_DisableDivisorAccess(LPC_USART);			/* divisor access must be disabled for IRQ setup, but this is already done in init */
  Chip_UART_IntEnable(LPC_USART, UART_IER_RBRINT);	/* enable receive interrupt */
  NVIC_EnableIRQ(UART0_IRQn);

  /* setup Baud rate */
  baud = Chip_UART_SetBaudFDR(LPC_USART, GPS_BAUD);	/* puuhh... this is a HUGE function */
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
  
  display_Write("Mount:\n");
  display_Write(gpx_get_sd_card_label());
  display_Write("\n");


  for(;;)
  {
    pq_ParseSentence(&pq);
    
    if ( is_trackpoint_save != 0 )
    {
      is_trackpoint_save = 0;
      if ( pq.gps_quality > 0 && pq.sat_cnt > 0 && pq.interface.pos.is_pos_and_time_update != 0 && pq.interface.pos.is_altitude_update != 0 )
      {
	if ( gpx_write(&(pq.interface.pos)) == 0 )
	{
	  display_Write("SD write failed\n");
	}
	else
	{
	  display_Write("SD write ok\n");
	}
	pq.interface.pos.is_altitude_update = 0;
	pq.interface.pos.is_pos_and_time_update = 0;
	is_sd_write_success = 1;
      }
      else
      {
	display_Write("GPS not avail.\n");	
      }
    }
    
    if ( is_send_gps_rate )
    {
      is_send_gps_rate  = 0;
      Chip_UART_SendBlocking(LPC_USART, gps_set_rate_cmd, sizeof(gps_set_rate_cmd));
    }
    

    if ( is_output_uart_data_cnt )
    {
      is_output_uart_data_cnt = 0;
      display_Write("UART: ");
      display_WriteUnsigned(uart_data_cnt);
      display_Write("\n");
    }
    
    if ( is_output_last_unknown_msg )
    {
      is_output_last_unknown_msg = 0;
      display_Write("? ");
      display_Write(pq.last_unknown_msg);
      display_Write("\n");
    }
    

    if ( is_output_sat_cnt )
    {
      is_output_sat_cnt = 0;
      display_Write("SatCnt ");
      display_WriteUnsigned(pq.sat_cnt);
      display_Write("\n");
    }
    
    if ( is_output_gps_quality ) 
    {
      is_output_gps_quality = 0;
      display_Write("Quality ");
      display_WriteUnsigned(pq.gps_quality);
      display_Write("\n");
    }
    
    if ( is_output_altitude )
    {
      int32_t alt = pq.interface.pos.altitude;
      is_output_altitude = 0;
      display_Write("Alt ");
      if ( alt < 0 )
      {
	display_Write("-");
	alt = -alt;
      }
      display_WriteUnsigned(alt);
      display_Write("m\n");
    }
    
    if ( is_output_pos )
    {
      is_output_pos = 0;
      display_Write("Lat ");
      display_WriteGpsFloat(pq.interface.pos.latitude);
      display_Write("\n");
      display_Write("Lon ");
      display_WriteGpsFloat(pq.interface.pos.longitude);
      display_Write("\n");
    }
    
    if ( is_output_time )
    {
      is_output_time = 0;
      display_Write("UTC ");
      display_WriteUnsigned(pq.interface.pos.hour);
      display_Write(":");
      display_WriteUnsigned(pq.interface.pos.minute);
      display_Write(":");
      display_WriteUnsigned(pq.interface.pos.second);
      display_Write("\n");
      
      //display_Write("Sec ");
      //display_WriteUnsigned(pq.interface.pos.time);
      //display_Write("\n");
    }

    if ( is_output_date )
    {
      is_output_date = 0;
      display_Write("Date ");
      display_WriteUnsigned(pq.interface.pos.day);
      display_Write(".");
      display_WriteUnsigned(pq.interface.pos.month);
      display_Write(".");
      display_WriteUnsigned(pq.interface.pos.year);
      display_Write("\n");
    }
    
    if ( is_output_digit_cnt )
    {
      is_output_digit_cnt = 0;
      display_Write("LatLonFrac  ");
      display_WriteUnsigned(pq.digit_cnt);
      display_Write("\n");
    }
    
    if ( is_output_gprmc_per_second )
    {
      is_output_gprmc_per_second = 0;
      display_Write("RMC ");
      display_WriteUnsigned(pq.valid_gprmc);
      display_Write("\n");
      
      display_Write("RMC/100S ");
      display_WriteUnsigned(((pq.valid_gprmc*(1000/SYS_TICK_PERIOD_IN_MS)*100)/sys_tick_irq_cnt));
      display_Write("\n");
    }

    


    
    
  }
  
  
  /* enter sleep mode: Reduce from 1.4mA to 0.8mA with 12MHz */  
  while (1)
  {
    SCB->SCR |= (1UL << SCB_SCR_SLEEPONEXIT_Pos);		/* enter sleep mode after interrupt */ 
    Chip_PMU_SleepState(LPC_PMU);						/* enter sleep mode now */
  }
}

