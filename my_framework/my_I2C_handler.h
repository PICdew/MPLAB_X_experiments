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
   
   bool moduleIsValid(I2C_MODULE modID);
   bool setupI2C(I2C_MODULE modID);
   bool StartTransferWithoutRestart(I2C_MODULE modID);
   bool StartTransferWithRestart(I2C_MODULE modID);
   bool StopTransfer(I2C_MODULE modID);
   bool TransmitOneByte(I2C_MODULE modID, UINT8 data);
   bool ReceiveOneByte(I2C_MODULE modID, UINT8 *data);
   bool TransmitNBytes(I2C_MODULE modID, char *str, unsigned int bytesToSend);
   bool myI2CWriteToLine(I2C_MODULE modID, char* string, unsigned int lineNum);
   bool myI2CWriteDeviceRegister(I2C_MODULE modID, unsigned int devAddr, unsigned int regAddr, UINT8 dataByte);
   bool myI2CReadDeviceRegister(I2C_MODULE modID, unsigned int devAddr, unsigned int regAddr, UINT8 *dataByte);
   bool myI2CInitCLS(I2C_MODULE modID);
   bool myI2CInitTemp(I2C_MODULE modID);
   bool myI2CInitAccel(I2C_MODULE modID);
   bool myI2CInitGyro(I2C_MODULE modID);
   bool readTempInF(I2C_MODULE modID, float *fptr);
   bool readAccel(I2C_MODULE modID, ACCEL_DATA *argData);
   bool readGyro(I2C_MODULE modID, GYRO_DATA *argData);

private:
   my_i2c_handler();
};




#endif	/* MY_I2C_HANDLER_H */

