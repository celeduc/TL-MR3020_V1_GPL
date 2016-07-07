/* (C) Copyright 2004 Atheros Communications, Inc.
 * All rights reserved.
 * File Name: led.cpp
 * Description: PBC LED control functions
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "led.h"
    
enum led_status {OFF=0, YELLOW,GREEN,RED};

static int  g_pbc_stat = LED_OFF; ;
static int  g_stateChange=0;
void led_display (int pbc_status)
{
     g_pbc_stat = pbc_status;
	 g_stateChange = 1;
}

static void led_ctrl(int led_color)
{
    switch(led_color){
        case YELLOW:
             system("exec echo 3 >/proc/simple_config/tricolor_led");
             break;
        case GREEN:
             system("exec echo 2 >/proc/simple_config/tricolor_led");
             break;
        case RED:
             system("exec echo 1 >/proc/simple_config/tricolor_led");
             break;
        case OFF:
             system("exec echo 0 >/proc/simple_config/tricolor_led");
             break;
    }
}
    
void * led_loop(void *p_data)
{
    int led_color = OFF;
    int overlap_counter = 0;
	bool offed = false;
int error_ok_counter=0;
	int firstTime = 0;
    
    for(;;){
if (g_stateChange)
		{
			error_ok_counter = 0;
			g_stateChange=0;
		}
        switch (g_pbc_stat) {
             case LED_INPROGRESS:
 				 offed = false;
                  if(led_color){
                     led_ctrl(OFF);
                     led_color = OFF;
                     usleep(100*1000);
                  }else{
                     // led_ctrl(YELLOW);
                     // led_color=YELLOW;
                     led_ctrl(GREEN);
                     led_color=GREEN;
                     usleep(200*1000);
                  }
                  break;
             case LED_ERROR:
error_ok_counter++;
                  if(error_ok_counter > 60 * 10){
                   led_display(LED_OFF);
                    break;
                  }
  				 offed = false;
                  if(led_color){
                     led_ctrl(OFF);
                     led_color = OFF;
                     usleep(100*1000);
                  }else{
                     led_ctrl(RED);
                     led_color=RED;
                     usleep(100*1000);
                  }
                  break;
             case LED_OVERLAP:
error_ok_counter++;
                  if(error_ok_counter > 60 * 5){
                   led_display(LED_OFF);
                    break;
                  }
  				 offed = false;
                  overlap_counter++;
                  if((overlap_counter >10) && (overlap_counter <=15)){
                    led_ctrl(OFF);
                    if (overlap_counter ==15)
                        overlap_counter = 0;
                    break;
                  }
                  if(led_color){
                     led_ctrl(OFF);
                     led_color = OFF;
                     usleep(100);
                  }else{
                     led_ctrl(RED);
                     led_ctrl(OFF);
                     led_color=RED;
                     usleep(100*1000);
                  }

                  break;
             case LED_SUCCESS:
error_ok_counter++;
                  if(error_ok_counter > 60 * 5){
                   led_display(LED_OFF);
                    break;
                  }
  				 offed = false;
                  led_ctrl(GREEN);
                  usleep(200*1000);
                  break;
        
             case LED_OFF:			 	
			 default:
			 	  if (!offed)
			 	{
                  	led_ctrl(OFF);
					offed = true;
			 	}
				  usleep(300*1000);
                  break;
       }
    }
}




