
#include "my_I2C_handler.h"

extern "C"
{
#include <peripheral/ports.h>	// Enable port pins for input or output
#include <peripheral/system.h>	// Setup the system and perihperal clocks for best performance
#include <peripheral/i2c.h>     // for I2C stuff
#include <peripheral/timer.h>   // for timer stuff
#include <string.h>
//#include <assert.h>
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


#define SYSTEM_CLOCK            80000000

// Note: This is defined because the #pragma statements are not definitions,
// so we can't use FPBDIV, and therefore we define our own for our period
// calculations
#define PB_DIV              2

// Note: This is defined because Tx_PS_1_SOMEPRSCALAR is a bitshift meant for
// a control register, not the prescalar value itself
#define PS_256              256

// define the timer period constant for the delay timer
#define T1_TOGGLES_PER_SEC  1000
#define T1_TICK_PR          SYSTEM_CLOCK/PB_DIV/PS_256/T1_TOGGLES_PER_SEC
#define T1_OPEN_CONFIG      T1_ON | T1_SOURCE_INT | T1_PS_1_256

// these are the I2C addresses of the warious daisy chained pmods
#define I2C_ADDR_PMOD_TEMP      0x4B
#define TWI_ADDR_PMOD_CLS       0x48
#define I2C_ADDR_PMOD_ACL       0x1D
#define I2C_ADDR_PMOD_ACL_X0    0x32
#define I2C_ADDR_PMOD_ACL_X1    0x33
#define I2C_ADDR_PMOD_ACL_Y0    0x34
#define I2C_ADDR_PMOD_ACL_Y1    0x35
#define I2C_ADDR_PMOD_ACL_Z0    0x36
#define I2C_ADDR_PMOD_ACL_Z1    0x37
#define I2C_ADDR_PMOD_ACL_PWR   0x2D
#define I2C_ADDR_PMOD_GYRO      0x69    // apparently, SDO is connected to VCC
#define I2C_ADDR_PMOD_GYRO_XL   0x28
#define I2C_ADDR_PMOD_GYRO_XH   0x29
#define I2C_ADDR_PMOD_GYRO_YL   0x2A
#define I2C_ADDR_PMOD_GYRO_YH   0x2B
#define I2C_ADDR_PMOD_GYRO_ZL   0x2C
#define I2C_ADDR_PMOD_GYRO_ZH   0x2D
#define I2C_ADDR_PMOD_GYRO_CTRL_REG1   0x20


// define the frequency (??what kind of frequency? clock frequency? bit transfer frequency? byte transfer frequency??) at which an I2C module will operate
#define I2C_FREQ_1KHZ       100000

// Globals for setting up pmod CLS
// values in Digilent pmod CLS reference manual, pages 2 - 3
const char enable_display[] =  {27, '[', '3', 'e', '\0'};
const char set_cursor[] =      {27, '[', '1', 'c', '\0'};
const char home_cursor[] =     {27, '[', 'j', '\0'};
const char wrap_line[] =       {27, '[', '0', 'h', '\0'};
const char set_line_two[] =    {27, '[', '1', ';', '0', 'H', '\0'};

unsigned int gMillisecondsInOperation;

BOOL moduleIsValid(I2C_MODULE modID)
{
    if (modID != I2C1 && modID != I2C2)
    {
        // invalid module for this board; abort
        return FALSE;
    }

    return TRUE;
}

BOOL setupI2C(I2C_MODULE modID)
{
    // this value stores the return value of I2CSetFrequency(...), and it can
    // be used to compare the actual set frequency against the desired
    // frequency to check for discrepancies
    UINT32 actualClock;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Set the I2C baudrate, then enable the module
    actualClock = I2CSetFrequency(modID, SYSTEM_CLOCK, I2C_FREQ_1KHZ);
    I2CEnable(modID, TRUE);
}

BOOL StartTransferWithoutRestart(I2C_MODULE modID)
{
    // thrashable storage for a return value; used in testing
    unsigned int returnVal;

    // records the status of the I2C module while waiting for it to start
    I2C_STATUS status;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Wait for the bus to be idle, then start the transfer
    while(!I2CBusIsIdle(modID));

    returnVal = I2CStart(modID);
    if(I2C_SUCCESS != returnVal) { return FALSE; }

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(modID);

    } while ( !(status & I2C_START) );

    return TRUE;
}

BOOL StartTransferWithRestart(I2C_MODULE modID)
{
    // thrashable storage for a return value; used in testing
    unsigned int returnVal;

    // records the status of the I2C module while waiting for it to start
    I2C_STATUS status;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Send the Restart) signal (I2C module does not have to be idle)
    returnVal = I2CRepeatStart(modID);
    if(I2C_SUCCESS != returnVal) { return FALSE; }

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(modID);

    } while ( !(status & I2C_START) );

    return TRUE;
}

BOOL StopTransfer(I2C_MODULE modID)
{
    // records the status of the I2C module while waiting for it to stop
    I2C_STATUS status;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Send the Stop signal
    I2CStop(modID);

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(modID);

    } while ( !(status & I2C_STOP) );
}

BOOL TransmitOneByte(I2C_MODULE modID, UINT8 data)
{
    // thrashable storage for a return value; used in testing
    unsigned int returnVal;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Wait for the transmitter to be ready
    while(!I2CTransmitterIsReady(modID));

    // Transmit the byte
    returnVal = I2CSendByte(modID, data);
    if(I2C_SUCCESS != returnVal) { return FALSE; }

    // Wait for the transmission to finish
    while(!I2CTransmissionHasCompleted(modID));

    // look for the acknowledge bit
    if(!I2CByteWasAcknowledged(modID)) { return FALSE; }

    return TRUE;
}

BOOL ReceiveOneByte(I2C_MODULE modID, UINT8 *data)
{
    unsigned int timeoutStart;
    unsigned int I2CtimeoutMS = 1000;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // attempt to enable the I2C module's receiver
    /*
     * if the receiver does not enable properly, report an error, set the
     * argument to 0 (don't just leave it hanging), and return false
     */
    if(I2CReceiverEnable(modID, TRUE) != I2C_SUCCESS)
    {
        *data = 0;
        return FALSE;
    }

    // wait for data to be available, then assign it when it is;
    /*
     * Note: if, prior to calling this function, the desired slave device's I2C
     * address was not sent a signal with the master's READ bit set, then the
     * desired slave device will not send data, resulting in an infinite loop
     */
    while(!(I2CReceivedDataIsAvailable(modID)));
    *data = I2CGetByte(modID);

    return TRUE;
}

BOOL TransmitNBytes(I2C_MODULE modID, char *str, unsigned int bytesToSend)
{
    /*
     * This function performs no initialization of the I2C line or the intended
     * device.  This function is a wrapper for many TransmitOneByte(...) calls.
     */
    unsigned int byteCount;
    unsigned char c;
    BOOL attempt = FALSE;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // check that the number of bytes to send is not bogus
    if (bytesToSend > strlen(str)) { return FALSE; }

    // initialize local variables, then send the string one byte at a time
    byteCount = 0;
    c = *str;
    while(byteCount < bytesToSend)
    {
        // transmit the bytes
        TransmitOneByte(modID, c);
        byteCount++;
        c = *(str + byteCount);
    }

    // transmission successful
    return TRUE;
}

BOOL myI2CWriteToLine(I2C_MODULE modID, char* string, unsigned int lineNum)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // start the I2C module and signal the CLS
    while(!StartTransferWithoutRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, TWI_ADDR_PMOD_CLS, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }

    // send the cursor selection command, then send the string
    if (2 == lineNum)
    {
        if (!TransmitNBytes(modID, (char*)set_line_two, strlen(set_line_two))) { return FALSE; }
    }
    else
    {
        // not line two, so assume line 1
        if (!TransmitNBytes(modID, (char*)home_cursor, strlen(home_cursor))) { return FALSE; }
    }
    if (!TransmitNBytes(modID, string, strlen(string))) { return FALSE; }
    StopTransfer(modID);

    return TRUE;
}


BOOL myI2CWriteDeviceRegister(I2C_MODULE modID, unsigned int devAddr, unsigned int regAddr, UINT8 dataByte)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // send a start bit and ready the specified register on the specified device
    while(!StartTransferWithoutRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, devAddr, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte))
    {
        // stop the transmission and return false
        StopTransfer(modID);
        return FALSE;
    }
    if (!TransmitOneByte(modID, regAddr))
    {
        // stop the transmission and return false
        StopTransfer(modID);
        return FALSE;
    }
    if (!TransmitOneByte(modID, dataByte))
    {
        // stop the transmission and return false
        StopTransfer(modID);
        return FALSE;
    }

    // stop the transmission
    StopTransfer(modID);

    return TRUE;
}

BOOL myI2CReadDeviceRegister(I2C_MODULE modID, unsigned int devAddr, unsigned int regAddr, UINT8 *dataByte)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // send a start bit and ready the specified register on the specified device
    while(!StartTransferWithoutRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, devAddr, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte))
    {
        // stop the transmission and return false
        StopTransfer(modID);
        return FALSE;
    }
    if (!TransmitOneByte(modID, regAddr))
    {
        // stop the transmission and return false
        StopTransfer(modID);
        return FALSE;
    }

    // now read that register
    while(!StartTransferWithRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, devAddr, I2C_READ);
    if (!TransmitOneByte(modID, SlaveAddress.byte))
    {
        // stop the transmission and return false
        StopTransfer(modID);
        return FALSE;
    }
    if (!ReceiveOneByte(modID, dataByte))
    {
        // stop the transmission and return false
        StopTransfer(modID);
        return FALSE;
    }

    // stop the transmission
    StopTransfer(modID);

    return TRUE;
}

BOOL myI2CInitCLS(I2C_MODULE modID)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // start the I2C module, signal the CLS, and send setting strings
    while(!StartTransferWithoutRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, TWI_ADDR_PMOD_CLS, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)enable_display, strlen(enable_display))) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)set_cursor, strlen(set_cursor))) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)home_cursor, strlen(home_cursor))) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)wrap_line, strlen(wrap_line))) { return FALSE; }
    StopTransfer(modID);

    // all went well, so return true
    return TRUE;
}

BOOL myI2CInitTemp(I2C_MODULE modID)
{
    // actually, with this temperature pmod, there is nothing to initialize;
    // keep this function in case you need to do something

    // all went well, so return true
    return TRUE;
}

BOOL myI2CInitAccel(I2C_MODULE modID)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;
    UINT8               dataByte;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // send a start bit
    while(!StartTransferWithoutRestart(modID));

    // read POWER_CTL register
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, I2C_ADDR_PMOD_ACL, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!TransmitOneByte(modID, I2C_ADDR_PMOD_ACL_PWR)) { return FALSE; }
    while(!StartTransferWithRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, I2C_ADDR_PMOD_ACL, I2C_READ);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!ReceiveOneByte(modID, &dataByte)) { return FALSE; }

    // set standby/measure bit to measure and write it back
    dataByte |= 0x08;
    while(!StartTransferWithRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, I2C_ADDR_PMOD_ACL, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!TransmitOneByte(modID, I2C_ADDR_PMOD_ACL_PWR)) { return FALSE; }
    if (!TransmitOneByte(modID, dataByte)) { return FALSE; }

    // stop the transmission
    StopTransfer(modID);

    // all went well, so return true
    return TRUE;
}

BOOL myI2CInitGyro(I2C_MODULE modID)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;
    UINT8               dataByte;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // read the control register that controls the power mode
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_CTRL_REG1, &dataByte)) { return FALSE; }

    // set control register bit 4 from low power mode (0) to normal mode (1)
    dataByte |= 0x08;

    // write back the modified control register
    if (!myI2CWriteDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_CTRL_REG1, dataByte)) { return FALSE; }

/*
    // send a start bit
    while(!StartTransferWithoutRestart(modID));

    // read POWER_CTL register
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, I2C_ADDR_PMOD_GYRO, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!TransmitOneByte(modID, I2C_ADDR_PMOD_GYRO_CTRL_REG1)) { return FALSE; }
    while(!StartTransferWithRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, I2C_ADDR_PMOD_GYRO, I2C_READ);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!ReceiveOneByte(modID, &dataByte)) { return FALSE; }

    // set control register bit 4 from low power mode (0) to normal mode (1)
    dataByte |= 0x08;
    while(!StartTransferWithRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, I2C_ADDR_PMOD_GYRO, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!TransmitOneByte(modID, I2C_ADDR_PMOD_GYRO_CTRL_REG1)) { return FALSE; }
    if (!TransmitOneByte(modID, dataByte)) { return FALSE; }

    // stop the transmission
    StopTransfer(modID);
*/

    // all went well, so return true
    return TRUE;
}


BOOL readTempInF(I2C_MODULE modID, float *fptr)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;
    UINT8               dataByte;
    UINT32              dataInt;
    float               temperature;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // send a start signal to the I2C module
    while(!StartTransferWithoutRestart(modID));

    // prepare the temperature pmod for reading
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, I2C_ADDR_PMOD_TEMP, I2C_READ);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }

    // the first read gets the high byte
    if (!ReceiveOneByte(modID, &dataByte)) { return FALSE; }
    dataInt = dataByte << 8;

    // the second read gets the low byte
    if (!ReceiveOneByte(modID, &dataByte)) { return FALSE; }
    dataInt |= dataByte;

    // convert the bit signal into degrees C according to the reference manual
    temperature = (dataInt >> 3) * 0.0625;

    // convert the termperature to degrees F
    temperature = ((temperature * 9) / 5) + 32;

    StopTransfer(modID);

    *fptr = temperature;

    return TRUE;
}

BOOL readAccel(I2C_MODULE modID, ACCEL_DATA *argData)
{
    INT16 localX;
    INT16 localY;
    INT16 localZ;
    UINT8 dataByte;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // read high byte of X register, then the low byte
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_X1, &dataByte)) { return FALSE; }
    localX = dataByte << 8;

    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_X0, &dataByte)) { return FALSE; }
    localX |= dataByte;

    // read high byte of Y register, then the low byte
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Y1, &dataByte)) { return FALSE; }
    localY = dataByte << 8;

    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Y0, &dataByte)) { return FALSE; }
    localY |= dataByte;

    // read high byte of Z register, then the low byte
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Z1, &dataByte)) { return FALSE; }
    localZ = dataByte << 8;

    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Z0, &dataByte)) { return FALSE; }
    localZ |= dataByte;

    // all data gathered successfully, so now multiply the data by the
    // conversion factor (retrieved from Josh Sackos' PmodACL.h) and put it
    // into the argument structure
    argData->X = (float)localX * (4.0 / 1024.0);
    argData->Y = (float)localY * (4.0 / 1024.0);
    argData->Z = (float)localZ * (4.0 / 1024.0);

    return TRUE;
}

BOOL readGyro(I2C_MODULE modID, GYRO_DATA *argData)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;
    UINT8 regAddr;

    INT16 localX;
    INT16 localY;
    INT16 localZ;
    UINT8 dataByte;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // read high byte of X register, then the low byte
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_XH, &dataByte)) { return FALSE; }
    localX = dataByte << 8;
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_XL, &dataByte)) { return FALSE; }
    localX |= dataByte;

    // read high byte of Y register, then the low byte
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_YH, &dataByte)) { return FALSE; }
    localY = dataByte << 8;
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_YL, &dataByte)) { return FALSE; }
    localY |= dataByte;

    // read high byte of Z register, then the low byte
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_ZH, &dataByte)) { return FALSE; }
    localZ = dataByte << 8;
    if (!myI2CReadDeviceRegister(modID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_ZL, &dataByte)) { return FALSE; }
    localZ |= dataByte;

    // all data gathered successfully, so now multiply the data by the
    // conversion factor (retrieved from Josh Sackos' PmodACL.h) and put it
    // into the argument structure
    argData->X = (float)localX * (500.0/65536.0);// * (4.0 / 1024.0);
    argData->Y = (float)localY * (500.0/65536.0);// * (4.0 / 1024.0);
    argData->Z = (float)localZ * (500.0/65536.0);// * (4.0 / 1024.0);

    // stop the transmission
    StopTransfer(modID);

    return TRUE;
}

