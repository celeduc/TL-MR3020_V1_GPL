/* (C) Copyright 2004 Atheros Communications, Inc.
 * All rights reserved.
 * File Name: led.h
 * Description: PBC LED control API
 */

#ifndef _WSC_LED_
#define _WSC_LED_

enum pbc_stat{
    LED_OFF=0,
    LED_INPROGRESS,
    LED_ERROR,
    LED_OVERLAP,
    LED_SUCCESS
};

extern void led_display (int pbc_status);

extern void *led_loop(void *p_data);

#endif //_WSC_LED_
