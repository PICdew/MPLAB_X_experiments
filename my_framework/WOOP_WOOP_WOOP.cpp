#include "WOOP_WOOP_WOOP.h"
#include "my_delay_timer.h"

extern "C" 
{
#include <peripheral/ports.h>
}


void WOOP_WOOP_WOOP(void)
{
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);
   PORTClearBits(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);

   // assume that the delay timer has not been initialized yet, and initialize
   // it with a 40MHz peripheral bus clock
   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
   delay_timer_ref.init(80000000 / 2);
   INTEnableSystemMultiVectoredInt();

   while(1)
   {
      PORTClearBits(IOPORT_B, BIT_11 | BIT_13);
      PORTSetBits(IOPORT_B, BIT_10 | BIT_12);
      delay_timer_ref.delay_ms(100);

      PORTClearBits(IOPORT_B, BIT_10 | BIT_12);
      PORTSetBits(IOPORT_B, BIT_11 | BIT_13);
      delay_timer_ref.delay_ms(100);
   }
}
