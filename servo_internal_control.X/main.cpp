extern "C"
{
#include <peripheral/ports.h>	// Enable port pins for input or output
//#include <peripheral/system.h>	// Setup the system and perihperal clocks for best performance
#include <peripheral/i2c.h>
#include <peripheral/timer.h>
#include <string.h>
}

#include "my_sys_config.h"
#include "../my_framework/my_I2C_handler.h"
#include "../my_framework/my_delay_timer.h"
#include "../my_framework/WOOP_WOOP_WOOP.h"


// Oscillator Settings
/*
 * SYSCLK = 80 MHz (8 MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
 * PBCLK = 40 MHz
 * Primary Osc w/PLL (XT+,HS+,EC+PLL)
 * WDT OFF
 * Other options are don't care
 */
#pragma config FNOSC = PRIPLL       // Oscillator selection
#pragma config POSCMOD = EC         // Primary oscillator mode
#pragma config FPLLIDIV = DIV_2     // PLL input divider
#pragma config FPLLMUL = MUL_20     // PLL multiplier
#pragma config FPLLODIV = DIV_1     // PLL output divider
#pragma config FPBDIV = DIV_2       // Peripheral bus clock divider
#pragma config FSOSCEN = OFF        // Secondary oscillator enable


// define the timer period for the servo timer
#define T2_TOGGLES_PER_SEC  10000
#define T2_PS              32
#define T2_TICK_PR          SYS_CLOCK/PB_DIV/T2_PS/T2_TOGGLES_PER_SEC
#define T2_OPEN_CONFIG      T2_ON | T2_SOURCE_INT | T2_PS_1_32


int desired_rotation_degrees;

extern "C" void __ISR(_TIMER_2_VECTOR, IPL7AUTO) Timer2Handler(void)
{
   static int counter = 0;
   static int prev_desired_rotation_degrees = 0;
   static int counter_limit = 0;
   int microsecond_offset_from_1500 = 0;

   // According to this wikipedia article and the accompanying picture, pulse
   // width modulation (PWM) is defined as activity over a 20ms interval.
   // Standard servos are controlled by providing a PWM signal between 1000
   // microseconds and 2000 microsecond pulse once every 20ms.  A 1000
   // microsecond pulse will make a standard servo -90 degrees from its neutral
   // position, a 1500 microsecond pulse will make it move to the 0 degree
   // position, and a 2000 microsecond pulse will make it move to the +90
   // degree position.  Anywhere in between those values will cause the servo
   // to move linearly somewhere in the middle (that is, a 1250 microsecond
   // pulse will make it move -45 degrees from neutral, and a 1750 microsecond
   // pulse will make it move +45 degrees from neutral).
   //
   // Thus, millisecond resolution for this timer is not appropriate.
   //
   // Note: The minimum pulse width (that is, for the minimum rotation of the
   // servo, which in our case in -90 degrees) and the maximum pulse width
   // (that is, for the maximum rotation of the servo, which in our case is +90
   // degrees) will vary for each servo.
   //
   // Rule of thumb is that minimum pulse duration is 1000 microseconds and
   // maximum pulse duration is 2000 microseconds.
   //
   // http://en.wikipedia.org/wiki/Servo_control,
   // http://en.wikipedia.org/wiki/File:Sinais_controle_servomotor.JPG


   // we have a 0.1 millisecond timer (this interrupt will go off 10,000 times
   // per second according to the tiemr configuration), so we will be able to
   // have (500 microsecond range for -90 OR +90 / 100 microsecond timer
   // frequency) = 10 distinct positions.

   if (desired_rotation_degrees != prev_desired_rotation_degrees)
   {
      microsecond_offset_from_1500 = desired_rotation_degrees * 500;
      microsecond_offset_from_1500 /= 90;
      counter_limit = (1500 + microsecond_offset_from_1500) / 100;

      prev_desired_rotation_degrees = desired_rotation_degrees;
   }

   //counter_limit = 20;
   if (counter < counter_limit)
   {
      PORTSetBits(IOPORT_G, BIT_12);
      counter += 1;
   }
   else
   {
      PORTClearBits(IOPORT_G, BIT_12);
      counter += 1;

      if (counter == 200)
      {
         counter = 0;
      }
   }

    // clear the interrupt flag
    mT2ClearIntFlag();
}


int main(void)
{
   bool ret_val = false;

   int i = 0;
   char message[20];

   OpenTimer2(T2_OPEN_CONFIG, T2_TICK_PR);
   ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);

   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
   delay_timer_ref.init(SYS_CLOCK / PB_DIV);
   INTEnableSystemMultiVectoredInt();

   my_i2c_handler i2c_driver = my_i2c_handler::get_instance();
   ret_val = i2c_driver.I2C_init(I2C2, SYS_CLOCK / PB_DIV);
   if (!ret_val)
   {
      WOOP_WOOP_WOOP();
   }

   ret_val = i2c_driver.CLS_init(I2C2);
   if (!ret_val)
   {
      WOOP_WOOP_WOOP();
   }

   ret_val = i2c_driver.CLS_write_to_line(I2C2, "CLS initialized", 1);
   if (!ret_val)
   {
      WOOP_WOOP_WOOP();
   }

   // turn an LED on and off for a little light show because...reasons
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10);
   PORTClearBits(IOPORT_B, BIT_10);

   // activate the pin that controls servo output #1 on the MX4cK
   PORTSetPinsDigitalOut(IOPORT_G, BIT_12);
   PORTClearBits(IOPORT_G, BIT_12);

   INTEnableSystemMultiVectoredInt();

   desired_rotation_degrees = -90;

   // loop forever
   i = 0;
   bool LED_is_on = false;
   while(1)
   {
      delay_timer_ref.delay_ms(500);

      snprintf(message, CLS_LINE_SIZE, "'%d'", desired_rotation_degrees);
      i2c_driver.CLS_write_to_line(I2C2, message, 1);
      
      desired_rotation_degrees += 10;
      if (desired_rotation_degrees > 90)
      {
         desired_rotation_degrees = -90;
      }


      if (LED_is_on)
      {
         PORTClearBits(IOPORT_B, BIT_10);
         LED_is_on = false;
      }
      else
      {
         PORTSetBits(IOPORT_B, BIT_10);
         LED_is_on = true;
      }

      i++;
   }
}
