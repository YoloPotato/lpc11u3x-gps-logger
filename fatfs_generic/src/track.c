/*

  track.c
  
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