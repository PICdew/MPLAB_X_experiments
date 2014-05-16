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
#include "my_sys_config.h"
#include "transceiver_control.h"


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


unsigned int g_milliseconds_in_operation;

// timer 1 is a millisecond timer that controls the other timers and doubles as
// a delay timer
extern "C" void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
   g_milliseconds_in_operation++;

   // clear the interrupt flag
   mT1ClearIntFlag();
}

void delay_ms(unsigned int milliseconds)
{
   unsigned int millisecond_count = g_milliseconds_in_operation;
   while ((g_milliseconds_in_operation - millisecond_count) < milliseconds);
}



// -----------------------------------------------------------------------------
//                    Main
// -----------------------------------------------------------------------------
int main(void)
{
   int i = 0;
   char message[CLS_LINE_SIZE];

   // open the timers, but do not turn on their interrupts yet
   OpenTimer1(T1_OPEN_CONFIG, T1_TICK_PR);
   ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);

   transceiver_control transceiver_control_ref = transceiver_control::get_instance();
   transceiver_control_ref.init();
   

   // MUST be performed before initializing the TCPIP stack, which uses an SPI
   // interrupt to communicate with the wifi pmod
   INTEnableSystemMultiVectoredInt();
   INTEnableInterrupts();

   // ---------------------------- Setpu LEDs ---------------------------------
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);
   PORTClearBits(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);

   my_i2c_handler i2c_ref = my_i2c_handler::get_instance();
   i2c_ref.I2C_init(I2C2, SYSTEM_CLOCK / PB_DIV);
   i2c_ref.CLS_init(I2C2);

   snprintf(message, CLS_LINE_SIZE, "I2C init good");
   i2c_ref.CLS_write_to_line(I2C2, message, 1);


   while(1)
   {
      transceiver_control_ref.do_not_return_until_start_of_20_ms_pwm_cycle();
      PORTToggleBits(IOPORT_B, BIT_12);

      snprintf(message, CLS_LINE_SIZE, "i = %d", i);
      i2c_ref.CLS_write_to_line(I2C2, message, 2);
      i += 1;



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

      //delay_ms(100);
//      ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
//      delayMS(1000);
//      ConfigIntTimer2(T2_INT_OFF | T2_INT_PRIOR_2);
   }


   return 0;
}


