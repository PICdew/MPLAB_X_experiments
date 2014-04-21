/*
 * File:   sys_config.h
 * Author: John
 *
 * Created on April 21, 2014, 11:40 AM
 */

#ifndef SYS_CONFIG_H
#define	SYS_CONFIG_H

extern "C"
{
#include <peripheral/ports.h>
}

// Let compile time pre-processor calculate the timer tick periods (PR)
#define SYS_FREQ              (80000000L)
#define PB_DIV                8
#define PRESCALE              256
#define T1_TOGGLES_PER_SEC    1000
#define T2_TOGGLES_PER_SEC    3
#define T3_TOGGLES_PER_SEC    6
#define T1_TICK_PR            (SYS_FREQ/PB_DIV/PRESCALE/T1_TOGGLES_PER_SEC)
#define T2_TICK_PR            (SYS_FREQ/PB_DIV/PRESCALE/T2_TOGGLES_PER_SEC)
#define T3_TICK_PR            (SYS_FREQ/PB_DIV/PRESCALE/T2_TOGGLES_PER_SEC)

#define LED_PORT              IOPORT_B
#define LED_1_PIN             BIT_10
#define LED_2_PIN             BIT_13

#define BUTTON_PORT           IOPORT_A
#define BUTTON_1_PIN          BIT_6
#define BUTTON_2_PIN          BIT_7
const unsigned int BUTTON_READ_DELAY = 500;



#endif	/* SYS_CONFIG_H */

