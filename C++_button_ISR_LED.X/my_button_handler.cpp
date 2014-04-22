// for the linker to hook up these function definitions with their declarations
#include "my_button_handler.h"

// for the IO port and pin defines
#include "my_sys_config.h"

// for disabling button reading for a time
#include "../my_framework/my_delay_timer.h"

// XC32 libraries
extern "C"
{
#include <peripheral/timer.h>
#include <peripheral/ports.h>
}

my_button_handler::my_button_handler()
{
   // set the button pins so that you can read from them
   PORTSetPinsDigitalIn(BUTTON_PORT, BUTTON_1_PIN | BUTTON_2_PIN);

   // configure the pins for LEDs LD1 and LD4 as digital outputs, and clear the
   // bits so that the LEDs start off

   PORTSetPinsDigitalOut(LED_PORT, LED_1_PIN | BIT_11 | BIT_12 | LED_2_PIN);
   PORTClearBits(LED_PORT, LED_1_PIN | BIT_11 | BIT_12 | LED_2_PIN);
}

my_button_handler& my_button_handler::get_instance(void)
{
   static my_button_handler MBH;

   return MBH;
}

void my_button_handler::do_the_button_thing(void)
{
   // read the buttons

   unsigned int port_state = 0;
   unsigned int button_states = 0;

   // employ some special delaying operations to prevent double readings from
   // the processor reading the ports too fast (a simple delay doesn't work
   // well and often results in ignored presses because most of the processor's
   // time is spent in dalys)
   // Note: I am decrementing a timer, and the most intuitive stopping point is
   // 0.  I am decrementing the timer, and I don't want a decrementing to wrap
   // around if it goes past 0, so I use a signed integer.  Also, keep the
   // timer in a handler in a manageable range so it doesn't run the risk of
   // signed overflow.
   static int btn_1_read_delay_soft_ms_timer = 0;
   static int btn_2_read_delay_soft_ms_timer = 0;

   // decrement the soft timers if they haven't been reset for too long
   // Note: These are millisecond software (as opposed to hardware ISR) timers,
   // and 32 unsigned bits is enough to continuously increment without overflow
   // for ~49.7 days, but difference arithmetic is immune to unsigned integer
   // overflow, so don't worry about overflow here.
   static unsigned int previous_ms_read = 0;
   unsigned int current_ms_read = 0;
   unsigned int diff_ms_read = 0;

   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();

   current_ms_read = delay_timer_ref.get_elapsed_time();
   diff_ms_read = current_ms_read - previous_ms_read;
   previous_ms_read = current_ms_read;


   // read the states of the pins feeding in from the buttons
   port_state = PORTRead(BUTTON_PORT);

   // mask out everything but the button pins
   button_states = port_state & (BUTTON_1_PIN | BUTTON_2_PIN);

   if (BUTTON_1_PIN == (BUTTON_1_PIN & button_states))
   {
      // button 1 pressed
      PORTSetBits(LED_PORT, BIT_11);

      if (btn_1_read_delay_soft_ms_timer <= 0)
      {
         // the button's handling timer has expired, so do what you need to do
         // as fast as you can
         btn_1_read_delay_soft_ms_timer = BUTTON_READ_DELAY;
         handle_button_1();
      }
      else
      {
         // button handling timer is still on cooldown, so do nothing
         // Note: If the button is held down, then the countdown doesn't happen
         // because the code comes into this else statement instead of the else
         // statement below that handles the countdown.
      }
   }
   else
   {
      PORTClearBits(LED_PORT, BIT_11);

      // button was either recently released or has not been pressed in a bit,
      // so do the cooldown thing
      if (btn_1_read_delay_soft_ms_timer > 0)
      {
         btn_1_read_delay_soft_ms_timer -= diff_ms_read;
      }
      else
      {
         // button handling timer has expired, but button is not pressed, so do
         // nothing
      }
   }

   if (BUTTON_2_PIN == (BUTTON_2_PIN & button_states))
   {
      PORTSetBits(LED_PORT, BIT_12);

      // button 2 pressed
      if (btn_2_read_delay_soft_ms_timer <= 0)
      {
         btn_2_read_delay_soft_ms_timer = BUTTON_READ_DELAY;
         handle_button_2();
      }
   }
   else
   {
      PORTClearBits(LED_PORT, BIT_12);

      // button was either recently released or has not been pressed in a bit
      if (btn_2_read_delay_soft_ms_timer > 0)
      {
         btn_2_read_delay_soft_ms_timer -= diff_ms_read;
      }
   }
}

void my_button_handler::handle_button_1(void)
{
   static bool button_timer_active = false;

   // pressing this button the first time turns on a timer that toggles an LED,
   // and pressing it again turns it off
   if (button_timer_active)
   {
      CloseTimer2();
      PORTClearBits(IOPORT_B, BIT_10);
      button_timer_active = false;
   }
   else
   {
      OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256, T2_TICK_PR);
      ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
      button_timer_active = true;
   }
}

void my_button_handler::handle_button_2(void)
{
   static bool button_timer_active = false;

   // same as "handle button 1", but with a different timer and different LED
   if (button_timer_active)
   {
      CloseTimer3();
      PORTClearBits(IOPORT_B, BIT_13);
      button_timer_active = false;
   }
   else
   {
      OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, T3_TICK_PR);
      ConfigIntTimer3(T3_INT_ON | T3_INT_PRIOR_2);
      button_timer_active = true;
   }
}

