/*

  gpx.c


https://en.wikipedia.org/wiki/GPS_Exchange_Format

http://www.topografix.com/GPX/1/1/




48.635910, 8.8932 75
48.6355 84, 8.8926 85
48.635573, 8.8925 94
48.635662, 8.892685

TP.time
In: TP, Threshold [s]

if gpx file does not exist then
   create gpx file
   write header to gpx file
   write trackstart to gpx file
   write trackend to gpx file
   write footer to gpx file
   lasttime = tp.time
   last time eeprom init
else
  lasttime = get last time from eeprom
end if

if tp.time - lasttime > Threshold then
  seek to end of track in gpx file
  write trackend to gpx file
  write trackstart to gpx file
  write trackend to gpx file
  write footer to gpx file
end if

seek to end of track in gpx file
write trackpoint to gpx file
write trackend to gpx file
write footer to gpx file
store last time to eeprom

====

eeprom:
init, get last time, store new time
*/

#include <string.h>
#include <chip.h>
#include "delay.h"
#include "u8g2.h"
#include "ff.h"		/* Declarations of FatFs API */
#include "eeprom.h"

FATFS FatFs;		/* FatFs work area needed for each volume */
FIL gpx_file;			/* File object needed for each open file */
const char gpx_name[] = "TRACK.GPX";
const char gpx_header[] = "<gpx version=\"1.1\" "\
"creator=\"https://github.com/olikraus/lpc11u3x-gps-logger\">\n"\
" <trk><name>LOG</name>\n";
const char gpx_footer[] = " </trk>\n</gpx>\n";
const char gpx_track_start[] = "  <trkseg>\n";
const char gpx_track_end[] = "  </trkseg>\n";


void gpx_log(const char *msg, FRESULT fr)
{
}

int gpx_mount(void)
{
  FRESULT fr;
  fr = f_mount(&FatFs, "", 1);		/* Give a work area to the default drive and force a mount (http://elm-chan.org/fsw/ff/en/mount.html) */
  if (   fr == FR_OK )
  {
    return 1;
  }
  gpx_log("mount failed", fr);
  return 0;    
}

/* 1 for ok, 0, for does not exist, -1 for error */
int gpx_is_available(void)
{
  FRESULT fr;
  fr = f_stat(gpx_name, NULL); 
  if ( fr == FR_OK) 
  {	
    return 1;
  }
  if ( fr ==  FR_NO_FILE) 
  {	
    return 0;
  }
  gpx_log("stat failed", fr);
  return -1;
}

int gpx_open_file(void)
{
  FRESULT fr;
  fr = f_open(&gpx_file, gpx_name, FA_WRITE | FA_OPEN_EXISTING); 
  if ( fr == FR_OK) 
  {	
    return 1;
  }
  gpx_log("open failed", fr);
  return -1;
}

int gpx_close_file(void)
{
  FRESULT fr;
  fr = f_close(&gpx_file);
  if ( fr == FR_OK )
  {
    return 1;
  }
  gpx_log("close failed", fr);
  return 0;
}


int gpx_write_header(void)
{
  UINT bw;
  FRESULT fr;
  fr = f_write(&gpx_file, gpx_header, strlen(gpx_header), &bw);
  if ( fr == FR_OK )
  {
    return 1;
  }
  gpx_log("wr header failed", fr);
  return 0;  
}

int gpx_write_footer(void)
{
  UINT bw;
  FRESULT fr;
  fr = f_write(&gpx_file, gpx_footer, strlen(gpx_footer), &bw);
  if ( fr == FR_OK )
  {
    return 1;
  }
  gpx_log("wr footer failed", fr);
  return 0;  
}

int gpx_write_track_start(void)
{
  UINT bw;
  FRESULT fr;
  fr = f_write(&gpx_file, gpx_track_start, strlen(gpx_track_start), &bw);
  if ( fr == FR_OK )
  {
    return 1;
  }
  gpx_log("wr trk start failed", fr);
  return 0;  
}


int gpx_write_track_end(void)
{
  UINT bw;
  FRESULT fr;
  fr = f_write(&gpx_file, gpx_track_end, strlen(gpx_track_end), &bw);
  if ( fr == FR_OK )
  {
    return 1;
  }
  gpx_log("wr trk end failed", fr);
  return 0;  
}

