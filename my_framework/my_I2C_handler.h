/*
 * File:   my_I2C_handler.h
 * Author: John
 *
 * Created on April 21, 2014, 5:40 PM
 */

#ifndef MY_I2C_HANDLER_H
#define	MY_I2C_HANDLER_H

extern "C"
{
#include <peripheral/i2c.h>
}

/*
 * Jumper setup for rev E CLS pmod
 *
 * MD0: shorted
 * MD1: shorted
 * MD2: open
 *
 * JP1: short for RST (shorting for SS will cause the display to not function
 * under TWI)
 *
 * J4: refers to one SCL pin and one SDA pin
 * J5: ditto with J4
 *
 * J6: refers to one VCC pin and one GND pin
 * J7: ditto with J6
 */

// for the CLS; used when formating strings to fit in a line
const unsigned int CLS_LINE_SIZE = 17;

 
class my_I2C_handler
{
public:
   static my_I2C_handler& get_instance(void);
   bool init(I2C_MODULE module_ID, unsigned int sys_clock, unsigned int desired_i2c_freq);
   
   // these are for the CLS pmod only
   bool CLS_init(void);
   bool CLS_write_to_line(const char* string, unsigned int lineNum);

private:
   my_I2C_handler(void);
   
   bool module_is_valid(I2C_MODULE modID);
   bool start_transfer_without_restart(void);
   bool start_transfer_with_restart(void);
   bool stop_transfer(void);
   bool transmit_one_byte(UINT8 data);
   bool transmit_n_bytes(const char *str, unsigned int bytesToSend);

   bool m_I2C_has_been_initialized;
   bool m_CLS_has_been_initialized;
   I2C_MODULE m_module_ID;
};

#endif	/* MY_I2C_HANDLER_H */

