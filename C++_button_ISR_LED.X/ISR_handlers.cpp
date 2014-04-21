// there is no header associated with this source file because the registration
// of the ISR handlers are XC32 macros

extern "C"
{
#include <peripheral/ports.h>
#include <peripheral/int.h>
}


// we are using the XC32 C++ compiler, but this ISR handler registration macro
// only seems to work with the XC32 C compiler, so we have to declare it as
// "extern"
extern "C" void __ISR(_TIMER_2_VECTOR, IPL7SOFT) timer_2_handler(void)
{
   // turn the LED on (if it is off) or off (if it is on)

   // the easy way
   //PORTToggleBits(IOPORT_B, BIT_10);

   // the longer way
   static bool is_on = false;
   if (is_on)
   {
       PORTClearBits(IOPORT_B, BIT_10);
       is_on = false;
   }
   else
   {
       PORTSetBits(IOPORT_B, BIT_10);
       is_on = true;
   }

   // clear the interrupt flag
   mT2ClearIntFlag();

}

extern "C" void __ISR(_TIMER_3_VECTOR, IPL7SOFT) timer_3_handler(void)
{
   // turn the LED on (if it is off) or off (if it is on)

   // the easy way
   PORTToggleBits(IOPORT_B, BIT_13);

   // the longer way
   static bool is_on = false;
   if (is_on)
   {
       PORTClearBits(IOPORT_B, BIT_13);
       is_on = false;
   }
   else
   {
       PORTSetBits(IOPORT_B, BIT_13);
       is_on = true;
   }
   
   // clear the interrupt flag
   mT3ClearIntFlag();
}



