// for linking the definition of the delay timer function to the declaration
#include "my_delay_timer.h"

extern "C"
{
#include <peripheral/int.h>
#include <peripheral/timer.h>
}

// this is a static global instead of a member because the interrupt that the 
// delay timer uses does not have access to class' private members, and I am 
// not going to make a public mutator that anyone can use, so I'll just make 
// it static to restrict it to the scope of this file, and call it good
static unsigned int g_milliseconds_in_operation;

my_delay_timer::my_delay_timer()
{
   g_milliseconds_in_operation = 0;
   m_has_been_initialized = false;
}

my_delay_timer& my_delay_timer::get_instance()
{
   static my_delay_timer ref;

   return ref;
}

void my_delay_timer::init(unsigned int pb_clock)
{
   if (m_has_been_initialized)
   {
      // initialization already happened, so do nothing and return
   }
   else
   {
      unsigned int ticks_per_sec = 1000;
      unsigned int t1_tick_period = pb_clock / 256 / ticks_per_sec;

      // activate the timer for my "delay milliseconds" function
      OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, t1_tick_period);
      ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_1);

      m_has_been_initialized = true;
   }
}

void my_delay_timer::delay_ms(unsigned int milliseconds)
{
   // only enter the following while(...) loop if the timer is up and running
   if (m_has_been_initialized)
   {
      unsigned int millisecondCount = g_milliseconds_in_operation;
      while((g_milliseconds_in_operation - millisecondCount) < milliseconds);
   }
}

unsigned int my_delay_timer::get_elapsed_ms_since_program_start(void)
{
   return g_milliseconds_in_operation;
}

bool my_delay_timer::timer_ms(unsigned int start_time_milliseconds, unsigned int timer_limit_ms)
{
   bool this_ret_val = false;

   if (g_milliseconds_in_operation - start_time_milliseconds > timer_limit_ms)
   {
      this_ret_val = true;
   }

   return this_ret_val;
}

// we are using the XC32 C++ compiler, but this ISR handler registration macro
// only seems to work with the XC32 C compiler, so we have to declare it as
// "extern"
// Note: The second argument to the macro was designed by Microchip to work
// with the compiler (or maybe it was the preprocessor; I don't know).  It is
// of the form "IPL" 
extern "C" void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
   g_milliseconds_in_operation++;

   // clear the interrupt flag
   mT1ClearIntFlag();
}

