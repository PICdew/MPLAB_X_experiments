/* 
 * File:   my_sys_config.h
 * Author: John
 *
 * Created on April 21, 2014, 12:50 PM
 */

#ifndef MY_SYS_CONFIG_H
#define	MY_SYS_CONFIG_H

extern "C"
{
#include <peripheral/ports.h>
}

// Let compile time pre-processor calculate the timer tick periods (PR)
#define SYS_FREQ              (80000000L)
#define PB_DIV                8
#define PRESCALE              256
#define T1_TOGGLES_PER_SEC    1000
#define T1_TICK_PR            (SYS_FREQ/PB_DIV/PRESCALE/T1_TOGGLES_PER_SEC)

#define LED_PORT              IOPORT_B
#define LED_1_PIN             BIT_10
#define LED_2_PIN             BIT_11
#define LED_3_PIN             BIT_12
#define LED_4_PIN             BIT_13

// the switch pmod is plugged into the top 6 pins of Digilent port JK (srsly,
// no jk'ing here) as a demonstration of using a physical port without all the
// pins being on the same PIC32 port
#define SWITCH_1_PORT         IOPORT_C
#define SWITCH_2_PORT         IOPORT_D
#define SWITCH_3_PORT         IOPORT_A
#define SWITCH_4_PORT         IOPORT_A
#define SWITCH_1_PIN          BIT_4
#define SWITCH_2_PIN          BIT_12
#define SWITCH_3_PIN          BIT_10
#define SWITCH_4_PIN          BIT_9

const unsigned int SWITCH_READ_DELAY = 200;


#endif	/* MY_SYS_CONFIG_H */

