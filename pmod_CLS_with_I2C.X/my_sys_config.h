/* 
 * File:   my_sys_config.h
 * Author: John
 *
 * Created on April 21, 2014, 7:18 PM
 */

#ifndef MY_SYS_CONFIG_H
#define	MY_SYS_CONFIG_H

#define SYS_CLOCK          80000000
#define PB_DIV             2
#define PS_256             256

// define the timer period constant for the delay timer
#define T1_TOGGLES_PER_SEC  1000
#define T1_TICK_PR          SYS_CLOCK/PB_DIV/PS_256/T1_TOGGLES_PER_SEC


#endif	/* MY_SYS_CONFIG_H */

