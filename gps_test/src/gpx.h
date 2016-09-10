/*

  gpx.h

*/


#ifndef _GPX_H
#define _GPX_H

#include "gps.h"

const char *gpx_get_sd_card_label(void);

/* write a gps pos to SD, returns 0 (fail) or 1 (ok) */
int gpx_write(gps_pos_t *pos);

#endif