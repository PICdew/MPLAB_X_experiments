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
    int i = 0;
    char message[20];
    my_I2C_handler i2c_driver = my_I2C_handler::get_instance();

    i2c_driver.init(I2C2, SYS_CLOCK, DESIRED_I2C_FREQ_1KHZ);
    i2c_driver.CLS_init();
    i2c_driver.CLS_write_to_line("CLS initialized", 1);

    my_delay_timer_init(SYS_CLOCK / PB_DIV);
    INTEnableSystemMultiVectoredInt();

    // loop forever
    i = 0;
    while(1)
    {
        snprintf(message, CLS_LINE_SIZE, "i = '%d'", i);
        i2c_driver.CLS_write_to_line(message, 1);

        i++;
        delay_ms(200);
    }
}
