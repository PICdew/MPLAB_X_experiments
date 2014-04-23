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

BOOL moduleIsValid(I2C_MODULE modID);
BOOL setupI2C(I2C_MODULE modID);
BOOL StartTransferWithoutRestart(I2C_MODULE modID);
BOOL StartTransferWithRestart(I2C_MODULE modID);
BOOL StopTransfer(I2C_MODULE modID);
BOOL TransmitOneByte(I2C_MODULE modID, UINT8 data);
BOOL ReceiveOneByte(I2C_MODULE modID, UINT8 *data);
BOOL TransmitNBytes(I2C_MODULE modID, char *str, unsigned int bytesToSend);
BOOL myI2CWriteToLine(I2C_MODULE modID, char* string, unsigned int lineNum);
BOOL myI2CWriteDeviceRegister(I2C_MODULE modID, unsigned int devAddr, unsigned int regAddr, UINT8 dataByte);
BOOL myI2CReadDeviceRegister(I2C_MODULE modID, unsigned int devAddr, unsigned int regAddr, UINT8 *dataByte);
BOOL myI2CInitCLS(I2C_MODULE modID);
BOOL myI2CInitTemp(I2C_MODULE modID);
BOOL myI2CInitAccel(I2C_MODULE modID);
BOOL myI2CInitGyro(I2C_MODULE modID);
BOOL readTempInF(I2C_MODULE modID, float *fptr);
BOOL readAccel(I2C_MODULE modID, ACCEL_DATA *argData);
BOOL readGyro(I2C_MODULE modID, GYRO_DATA *argData);



#endif	/* MY_I2C_HANDLER_H */

