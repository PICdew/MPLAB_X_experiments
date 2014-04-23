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

   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
   delay_timer_ref.init(SYS_CLOCK / PB_DIV);
   INTEnableSystemMultiVectoredInt();

   my_I2C_handler i2c_driver = my_I2C_handler::get_instance();
   ret_val = i2c_driver.init(I2C2, SYS_CLOCK, DESIRED_I2C_FREQ_1KHZ);

   ret_val = i2c_driver.CLS_init();

   ret_val = i2c_driver.CLS_write_to_line("CLS initialized", 1);

   ret_val = i2c_driver.acl_init();


   // turn an LED on and off for a little light show because...reasons
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10 | BIT_12 | BIT_13);
   PORTClearBits(IOPORT_B, BIT_10 | BIT_12 | BIT_13);

   // loop forever
   bool LED_is_on = false;
   ACCEL_DATA acl_data;
   char message[20];
   int i = 0;
   while(1)
   {
      ret_val = i2c_driver.acl_read(&acl_data);
      if (ret_val)
      {
         PORTToggleBits(IOPORT_B, BIT_12);
         PORTClearBits(IOPORT_B, BIT_13);
      }
      else
      {
         PORTToggleBits(IOPORT_B, BIT_13);
         PORTClearBits(IOPORT_B, BIT_12);
      }
//      snprintf(message, CLS_LINE_SIZE, "i = '%d'", i);
//      i2c_driver.CLS_write_to_line(message, 1);
//      i += 1;
//      ret_val = i2c_driver.acl_read(&acl_data);
//      if (ret_val)
//      {
//         snprintf(message, CLS_LINE_SIZE, "X=%5.2f;;Y=%5.2f", acl_data.X, acl_data.Y);
//         i2c_driver.CLS_write_to_line(message, 1);
//
//         snprintf(message, CLS_LINE_SIZE, "Z=%5.2f", acl_data.Z);
//         i2c_driver.CLS_write_to_line(message, 2);
//      }
//      else
//      {
//         snprintf(message, CLS_LINE_SIZE, "i = '%d'", i);
//         i2c_driver.CLS_write_to_line(message, 1);
//         i += 1;
//      }

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

      delay_timer_ref.delay_ms(200);
   }
}
