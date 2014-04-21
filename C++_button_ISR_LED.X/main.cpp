// much of this code, especially the #pragmas and #defines and system and timer
// configurations, were taken from the following file:
// C:\Program Files (x86)\Microchip\xc32\v1.20\examples\plib_examples\timer\timer1_int\source\timer1_int.c

extern "C"
{
#include <peripheral/system.h>
}

#include "my_button_handler.h"
#include "my_delay_timer.h"
#include "my_sys_config.h"

// Configuration Bit settings
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 40 MHz
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF  (??Watch Dog Timer??)
// Other options are don't care
//
#pragma config FPLLMUL = MUL_20
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLODIV = DIV_1
#pragma config FWDTEN = OFF
#pragma config POSCMOD = HS
#pragma config FNOSC = PRIPLL
#pragma config FPBDIV = DIV_8


int main(void)
{
   // remember to set a heap size in the project properties because C++ files
   // won't compile without it

   SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

   // initialize the things
   my_delay_timer_init();
   my_button_handler local_bh = my_button_handler::get_instance();

   // enable multi-vector interrupts
   INTEnableSystemMultiVectoredInt();

   while(1)
   {
      local_bh.do_the_button_thing();

      // delay a short time so that we don't read a single button press
      // multiple times, assuming of course that the user only intended to
      // quickly push the button once
      delay_ms(200);
   }
}
