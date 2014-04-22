/* 
 * File:   main.cpp
 * Author: John
 *
 * Created on April 21, 2014, 12:48 PM
 */

#include "my_sys_config.h"
extern "C"
{
#include <peripheral/system.h>
#include <peripheral/ports.h>
}

int main(int argc, char** argv)
{
   SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

   PORTSetPinsDigitalIn(SWITCH_1_PORT, SWITCH_1_PIN);
   PORTSetPinsDigitalIn(SWITCH_2_PORT, SWITCH_2_PIN);
   PORTSetPinsDigitalIn(SWITCH_3_PORT, SWITCH_3_PIN);
   PORTSetPinsDigitalIn(SWITCH_4_PORT, SWITCH_4_PIN);

   PORTSetPinsDigitalOut(LED_PORT,
           LED_1_PIN | LED_2_PIN | LED_3_PIN | LED_4_PIN);
   PORTClearBits(LED_PORT,
           LED_1_PIN | LED_2_PIN | LED_3_PIN | LED_4_PIN);

   // initialize the things
   my_delay_timer_init();

   while(1)
   {
      unsigned int switch_1_port_state = PORTRead(SWITCH_1_PORT);
      unsigned int switch_2_port_state = PORTRead(SWITCH_2_PORT);
      unsigned int switch_3_port_state = PORTRead(SWITCH_3_PORT);
      unsigned int switch_4_port_state = PORTRead(SWITCH_4_PORT);

      unsigned int switch_1_state = switch_1_port_state & SWITCH_1_PIN;
      unsigned int switch_2_state = switch_2_port_state & SWITCH_2_PIN;
      unsigned int switch_3_state = switch_3_port_state & SWITCH_3_PIN;
      unsigned int switch_4_state = switch_4_port_state & SWITCH_4_PIN;

      if (0 != switch_1_state)
      {
         PORTSetBits(SWITCH_1_PORT, SWITCH_1_PIN);
      }
      else
      {
         PORTClearBits(SWITCH_1_PORT, SWITCH_1_PIN);
      }

      if (0 != switch_2_state)
      {
         PORTSetBits(SWITCH_2_PORT, SWITCH_2_PIN);
      }
      else
      {
         PORTClearBits(SWITCH_2_PORT, SWITCH_2_PIN);
      }

      if (0 != switch_3_state)
      {
         PORTSetBits(SWITCH_3_PORT, SWITCH_3_PIN);
      }
      else
      {
         PORTClearBits(SWITCH_3_PORT, SWITCH_3_PIN);
      }

      if (0 != switch_4_state)
      {
         PORTSetBits(SWITCH_4_PORT, SWITCH_4_PIN);
      }
      else
      {
         PORTClearBits(SWITCH_4_PORT, SWITCH_4_PIN);
      }

   }

   return 0;
}

