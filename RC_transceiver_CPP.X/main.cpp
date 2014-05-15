/* 
 * File:   main.cpp
 * Author: John
 *
 * Created on May 15, 2014, 7:51 AM
 */

extern "C"
{
#include <peripheral/system.h>      // for ??
#include <peripheral/timer.h>       // for opening and configuring the timer
#include <peripheral/ports.h>       // for initializing IO pins
#include <peripheral/i2c.h>         // for I2C_MODULE typedef
#include <stdio.h>                  // for snprintf(...)
}

#include "../../github_my_MX4cK_framework/my_CPP_I2C_handler.h"


// ------------------ Configuration Oscillators --------------------------------
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK  = 40 MHz
// -----------------------------------------------------------------------------
#pragma config FPLLMUL = MUL_20
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLODIV = DIV_1
#pragma config FWDTEN = OFF
#pragma config POSCMOD = HS
#pragma config FNOSC = PRIPLL
#pragma config FPBDIV = DIV_2
//#pragma config CP = OFF, BWP = OFF, PWP = OFF
//#pragma config UPLLEN = OFF


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
#define T2_PS                 16
#define T2_TRIGGERS_PER_SEC   100000
#define T2_TICK_PR            SYSTEM_CLOCK/PB_DIV/T2_PS/T2_TRIGGERS_PER_SEC
#define T2_OPEN_CONFIG        T2_ON | T2_SOURCE_INT | T2_PS_1_16


#define RECEIVER_PIN_1  BIT_0
#define RECEIVER_PIN_2  BIT_1
#define RECEIVER_PIN_3  BIT_2
#define RECEIVER_PIN_4  BIT_3

unsigned int g_milliseconds_in_operation;
unsigned int g_10_microsecond_counter;
bool g_is_synchronized;

int g_receiver_pin_1_count_on;
int g_receiver_pin_1_count_off;
int g_receiver_pin_2_count_on;
int g_receiver_pin_2_count_off;
int g_receiver_pin_3_count_on;
int g_receiver_pin_3_count_off;
int g_receiver_pin_4_count_on;
int g_receiver_pin_4_count_off;

float g_receiver_pin_1_fractional_count_on;
float g_receiver_pin_2_fractional_count_on;
float g_receiver_pin_3_fractional_count_on;
float g_receiver_pin_4_fractional_count_on;

// make this interrupt handler save program state on the stack
// Note: The 100,000 timer is more demanding, so I want it to have the shadow 
// registers.  Therefore explicitly set this one to use software to save the 
// program state.
extern "C" void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
   g_milliseconds_in_operation++;

   switch (g_milliseconds_in_operation % 20)
   {
      case 0:
      {
         // beginning of PWM cycle, so turn the signal reading interrupt on
         break;
      }
      case 2:
      {
         // the PWM signals will be hi for 2ms at most, so turn off the signal
         // reading interrupt to conserve cpu cycles, then queue up the signal
         // processing function
      }
      case 5:
      {
         // queue up any any I2C communication in this interval
      }
   }

   if (0 == g_milliseconds_in_operation % 20)


   if (0 == (g_milliseconds_in_operation % 20))
   {
      g_receiver_pin_1_fractional_count_on = (float)g_receiver_pin_1_count_on / (g_receiver_pin_1_count_on + g_receiver_pin_1_count_off);
      g_receiver_pin_2_fractional_count_on = (float)g_receiver_pin_2_count_on / (g_receiver_pin_2_count_on + g_receiver_pin_2_count_off);
      g_receiver_pin_3_fractional_count_on = (float)g_receiver_pin_3_count_on / (g_receiver_pin_3_count_on + g_receiver_pin_3_count_off);
      g_receiver_pin_4_fractional_count_on = (float)g_receiver_pin_4_count_on / (g_receiver_pin_4_count_on + g_receiver_pin_4_count_off);

      g_receiver_pin_1_count_on = 0;
      g_receiver_pin_1_count_off = 0;
      g_receiver_pin_2_count_on = 0;
      g_receiver_pin_2_count_off = 0;
      g_receiver_pin_3_count_on = 0;
      g_receiver_pin_3_count_off = 0;
      g_receiver_pin_4_count_on = 0;
      g_receiver_pin_4_count_off = 0;
   }

   // clear the interrupt flag
   mT1ClearIntFlag();
}

unsigned int g_port_state;
unsigned int g_pin_state = 0;

// set this 100,000 triggers/sec timer to use the shadow registers for minimal
// strain on the system
extern "C" void __ISR(_TIMER_2_VECTOR, IPL7AUTO) Timer2Handler(void)
{
//   g_10_microsecond_counter += 1;
//
//   if (0 == (g_10_microsecond_counter % 10000))
//   {
//      PORTToggleBits(IOPORT_B, BIT_11);
//   }
   g_port_state = PORTRead(IOPORT_E) & (RECEIVER_PIN_1 | RECEIVER_PIN_2 | RECEIVER_PIN_3 | RECEIVER_PIN_4);

   if (!g_is_synchronized)
   {
      // not synchronized yet, so wait for the PWM signal to start its 20ms
      // cycle over again
      // Note: The PWM signal standard for servos and ESC states that the
      // signal will be high for the first millisecond in a 20ms cycle, then on
      // for a variable amount of time in the second millisecond, then low for
      // the remaining time in the cycle.  Rinse and repeat.  Therefore, to
      // synchronize, wait until the signal changes from low to high, which
      // indicates the start of the 20ms cycle
      static unsigned int prev_some_pin_state = 0;
      unsigned int curr_some_pin_state = g_port_state & RECEIVER_PIN_1;

      if ((curr_some_pin_state != 0) && (0 == prev_some_pin_state))
      {
         g_is_synchronized = true;
      }

      prev_some_pin_state = curr_some_pin_state;
   }
   else
   {
      //g_pin_state = g_port_state & RECEIVER_PIN_1;
      //if (g_pin_state != 0)
      if ((g_port_state & RECEIVER_PIN_1) != 0)
      {
         //g_receiver_pin_1_count_on += 1;
         PORTSetBits(IOPORT_G, BIT_12);
      }
      else
      {
         PORTClearBits(IOPORT_G, BIT_12);
         //g_receiver_pin_1_count_off += 1;
      }

      //g_pin_state = g_port_state & RECEIVER_PIN_2;
      //if (g_pin_state != 0)
      if ((g_port_state & RECEIVER_PIN_2) != 0)
      {
         g_receiver_pin_2_count_on += 1;
      }
      else
      {
         g_receiver_pin_2_count_off += 1;
      }

      //g_pin_state = g_port_state & RECEIVER_PIN_3;
      //if (g_pin_state != 0)
      if ((g_port_state & RECEIVER_PIN_3) != 0)
      {
         g_receiver_pin_3_count_on += 1;
      }
      else
      {
         g_receiver_pin_3_count_off += 1;
      }

      //g_pin_state = g_port_state & RECEIVER_PIN_4;
      //if (g_pin_state != 0)
      if ((g_port_state & RECEIVER_PIN_4) != 0)
      {
         g_receiver_pin_4_count_on += 1;
      }
      else
      {
         g_receiver_pin_4_count_off += 1;
      }
   }

   // clear the interrupt flag
   mT2ClearIntFlag();
}

void delayMS(unsigned int milliseconds)
{
   unsigned int millisecondCount = g_milliseconds_in_operation;
   while ((g_milliseconds_in_operation - millisecondCount) < milliseconds);
}


// -----------------------------------------------------------------------------
//                    Main
// -----------------------------------------------------------------------------
int main(void)
{
   int i = 0;
   char message[CLS_LINE_SIZE];
   g_milliseconds_in_operation = 0;
   g_10_microsecond_counter = 0;
   g_is_synchronized = false;
   g_port_state = 0;
   g_receiver_pin_1_count_on = 0;
   g_receiver_pin_1_count_off = 0;
   g_receiver_pin_2_count_on = 0;
   g_receiver_pin_2_count_off = 0;
   g_receiver_pin_3_count_on = 0;
   g_receiver_pin_3_count_off = 0;
   g_receiver_pin_4_count_on = 0;
   g_receiver_pin_4_count_off = 0;
   g_receiver_pin_1_fractional_count_on = 0;
   g_receiver_pin_2_fractional_count_on = 0;
   g_receiver_pin_3_fractional_count_on = 0;
   g_receiver_pin_4_fractional_count_on = 0;


   // open the timers, but do not turn on their interrupts yet
   OpenTimer1(T1_OPEN_CONFIG, T1_TICK_PR);
   OpenTimer2(T2_OPEN_CONFIG, T2_TICK_PR);

   // MUST be performed before initializing the TCPIP stack, which uses an SPI
   // interrupt to communicate with the wifi pmod
   INTEnableSystemMultiVectoredInt();
   INTEnableInterrupts();

   // ---------------------------- Setpu LEDs ---------------------------------
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);
   PORTClearBits(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);

   // open the port pins to read the receiver's data
   PORTSetPinsDigitalIn(IOPORT_E, RECEIVER_PIN_1 | RECEIVER_PIN_2);// | RECEIVER_PIN_3 | RECEIVER_PIN_4);

   PORTSetPinsDigitalOut(IOPORT_G, BIT_12);
   PORTClearBits(IOPORT_G, BIT_12);


   my_i2c_handler i2c_ref = my_i2c_handler::get_instance();
   i2c_ref.I2C_init(I2C2, SYSTEM_CLOCK / PB_DIV);
   i2c_ref.CLS_init(I2C2);

   snprintf(message, CLS_LINE_SIZE, "I2C init good");
   i2c_ref.CLS_write_to_line(I2C2, message, 1);

   // open the fast timer and attempt to synchronize with the receiver
   ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
   while(!g_is_synchronized);
   snprintf(message, CLS_LINE_SIZE, "receiver synchronized");
   i2c_ref.CLS_write_to_line(I2C2, message, 1);

   //ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);

   while(1)
   {
      PORTToggleBits(IOPORT_B, BIT_12);

      //snprintf(message, CLS_LINE_SIZE, "%x", g_port_state);
//      snprintf(message, CLS_LINE_SIZE, "1=%.3f|2=%.3f", g_receiver_pin_1_fractional_count_on, g_receiver_pin_2_fractional_count_on);
//      i2c_ref.CLS_write_to_line(I2C2, message, 1);
//      snprintf(message, CLS_LINE_SIZE, "3=%.3f|4=%.3f", g_receiver_pin_3_fractional_count_on, g_receiver_pin_4_fractional_count_on);
//      i2c_ref.CLS_write_to_line(I2C2, message, 2);

//      snprintf(message, CLS_LINE_SIZE, "x = '%d'", g_milliseconds_in_operation);
//      i2c_ref.CLS_write_to_line(I2C2, message, 1);

//      snprintf(message, CLS_LINE_SIZE, "y = '%d'", g_10_microsecond_counter);
//      i2c_ref.CLS_write_to_line(I2C2, message, 2);
//      i += 1;

      delayMS(100);
//      ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
//      delayMS(1000);
//      ConfigIntTimer2(T2_INT_OFF | T2_INT_PRIOR_2);
   }


   return 0;
}


