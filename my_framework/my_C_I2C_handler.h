/* 
 * File:   my_C_I2C_handler.h
 * Author: John
 *
 * Created on April 23, 2014, 7:11 PM
 */

#ifndef I2C_STUFF_H
#define	I2C_STUFF_H

#ifdef	__cplusplus
extern "C" {
#endif

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

#include <peripheral/i2c.h>

// for the CLS; used when formating strings to fit in a line
#define CLS_LINE_SIZE 17



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


#ifdef	__cplusplus
}
#endif

#endif	/* I2C_STUFF_H */

