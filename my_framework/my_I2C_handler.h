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

typedef struct accelData
{
   float X;
   float Y;
   float Z;
} ACCEL_DATA;

typedef struct gyroData
{
   float X;
   float Y;
   float Z;
} GYRO_DATA;

class my_i2c_handler
{
public:
   static my_i2c_handler& get_instance(void);
   
   bool I2C_init(I2C_MODULE module_ID, unsigned int pb_clock);
   bool CLS_init(I2C_MODULE module_ID);
   bool temp_init(I2C_MODULE module_ID);
   bool accel_init(I2C_MODULE module_ID);
   bool gyro_init(I2C_MODULE module_ID);

   bool CLS_write_to_line(I2C_MODULE module_ID, char* string, unsigned int lineNum);
   bool temp_read(I2C_MODULE module_ID, float *fptr);
   bool accel_read(I2C_MODULE module_ID, ACCEL_DATA *argData);
   bool gyro_read(I2C_MODULE module_ID, GYRO_DATA *argData);

private:
   my_i2c_handler();

   bool module_is_valid(I2C_MODULE module_ID);
   bool start_transfer(I2C_MODULE module_ID, bool start_with_restart);
   bool stop_transfer(I2C_MODULE module_ID);
   bool transmit_one_byte(I2C_MODULE module_ID, UINT8 data);
   bool receive_one_byte(I2C_MODULE module_ID, UINT8 *data);
   bool transmit_n_bytes(I2C_MODULE module_ID, char *str, unsigned int bytesToSend);
   bool write_device_register(I2C_MODULE module_ID, unsigned int devAddr, unsigned int regAddr, UINT8 dataByte);
   bool read_device_register(I2C_MODULE module_ID, unsigned int devAddr, unsigned int regAddr, UINT8 *dataByte);

   bool m_i2c_X_has_been_initialized[I2C_NUMBER_OF_MODULES];
   bool m_cls_on_i2c_X_has_been_initialized[I2C_NUMBER_OF_MODULES];
   bool m_temp_on_i2c_X_has_been_initialized[I2C_NUMBER_OF_MODULES];
   bool m_accel_on_i2c_X_has_been_initialized[I2C_NUMBER_OF_MODULES];
   bool m_gyro_on_i2c_X_has_been_initialized[I2C_NUMBER_OF_MODULES];
};





#endif	/* MY_I2C_HANDLER_H */

