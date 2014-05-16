/* 
 * File:   my_sys_config.h
 * Author: John
 *
 * Created on May 16, 2014, 8:13 AM
 */

#ifndef MY_SYS_CONFIG_H
#define	MY_SYS_CONFIG_H

extern "C"
{
#include <peripheral/timer.h>    // for timer related #defines
#include <peripheral/ports.h>    // for port and bit number #defines
}

#define SYSTEM_CLOCK          80000000
#define PB_DIV                2

// set the "function queing" and "delay" timer interrupt to go off 1000 times
// per second
// Note: I chose a prescale of 64 because 80,000,000 / 2 / 64 / 1000 is a whole
// number.
#define T1_PS                 64
#define T1_TRIGGERS_PER_SEC   1000
#define T1_TICK_PR            SYSTEM_CLOCK/PB_DIV/T1_PS/T1_TRIGGERS_PER_SEC
#define T1_OPEN_CONFIG        T1_ON | T1_SOURCE_INT | T1_PS_1_64

// set the "PWM reading" timer interrupt to go off 100,000 times per second
// Note: I chose a prescale of 64 because 80,000,000 / 2 / 16 / 100,000 is a
// whole number.
// Note: This timer is quite demanding on the system because of how often it
// triggers, so it will only be selectively turned on and off.
#define TRANSCEIVER_TIMERS_TRIGGERS_PER_SEC   10000

#define T2_PS                 16
#define T2_TICK_PR            SYSTEM_CLOCK/PB_DIV/T2_PS/TRANSCEIVER_TIMERS_TRIGGERS_PER_SEC
#define T2_OPEN_CONFIG        T2_ON | T2_SOURCE_INT | T2_PS_1_16
#define T2_INT_CONFIG_ON      T2_INT_ON | T2_INT_PRIOR_1
#define T2_INT_CONFIG_OFF     T2_INT_OFF

#define RECEIVER_IOPORT    IOPORT_E
#define RECEIVER_PIN_1     BIT_0
#define RECEIVER_PIN_2     BIT_1
#define RECEIVER_PIN_3     BIT_2
#define RECEIVER_PIN_4     BIT_3
#define NUM_RECEIVER_PINS  4

#define SERVO_IOPORT          IOPORT_G
#define SERVO_1_PIN           BIT_12
#define NUM_SERVO_PINS        1

#define MOTOR_IOPORT          IOPORT_G
#define MOTOR_1_PIN           BIT_12
#define NUM_MOTOR_PINS        1

#endif	/* MY_SYS_CONFIG_H */

