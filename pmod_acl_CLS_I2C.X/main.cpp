extern "C"
{
#include <peripheral/ports.h>	// Enable port pins for input or output
#include <peripheral/system.h>	// Setup the system and perihperal clocks for best performance
#include <peripheral/i2c.h>
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


int main(void)
{
   bool ret_val = false;

   int i = 0;
   char message[20];

   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
   delay_timer_ref.init(SYS_CLOCK / PB_DIV);
   INTEnableSystemMultiVectoredInt();

   my_I2C_handler i2c_driver = my_I2C_handler::get_instance();
   ret_val = i2c_driver.init(I2C2, SYS_CLOCK / PB_DIV, DESIRED_I2C_FREQ_1KHZ);
   if (!ret_val)
   {
      WOOP_WOOP_WOOP();
   }

   ret_val = i2c_driver.CLS_init();
   if (!ret_val)
   {
      WOOP_WOOP_WOOP();
   }

   ret_val = i2c_driver.CLS_write_to_line("CLS initialized", 1);
   if (!ret_val)
   {
      WOOP_WOOP_WOOP();
   }

   ret_val = i2c_driver.temp_init();
   if (!ret_val)
   {
      WOOP_WOOP_WOOP();
   }

   // turn an LED on and off for a little light show because...reasons
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10);
   PORTClearBits(IOPORT_B, BIT_10);

   // loop forever
   i = 0;
   bool LED_is_on = false;
   float temp = 0.0f;
   while(1)
   {
      ret_val = i2c_driver.temp_read(&temp, true);
      if (ret_val)
      {
         snprintf(message, CLS_LINE_SIZE, "temp = '%.3f'", temp);
      }
      else
      {
         snprintf(message, CLS_LINE_SIZE, "temp = XXXX");
      }

      i2c_driver.CLS_write_to_line(message, 1);

      snprintf(message, CLS_LINE_SIZE, "i = '%d'", i);
      i2c_driver.CLS_write_to_line(message, 2);

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
      delay_timer_ref.delay_ms(200);
   }
}
