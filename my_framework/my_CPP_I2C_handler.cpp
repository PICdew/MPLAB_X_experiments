///////////////////////////////////////////////////////////////////////////////
/// @file
/// @author    John Cox 
/// @brief     This file contains my I2C code, but unlike the C version, this
///            one encapsulates it into a class.
///////////////////////////////////////////////////////////////////////////////

#include "my_CPP_I2C_handler.h"

extern "C"
{
#include <peripheral/ports.h>    // Enable port pins for input or output
#include <peripheral/system.h>   // Setup the system and perihperal clocks for best performance
#include <peripheral/i2c.h>      // for I2C stuff
#include <string.h>              // for ???
}


///////////////////////////////////////////////////////////////////////////////
/// @brief
///   Jumper setup for CLS pmod rev E:
///   - MD0: shorted
///   - MD1: shorted
///   - MD2: open
///   - JP1: short for RST (shorting for SS will cause the display to not
///      function under I2C)
///   - J4: Is a label for one SCL pin and one SDA pin.  Connect MX4cK I2C
///      line's SCL and SDA pins to these.
///   - J5: Another set of one SCL pin and one SDA pin.  Use for daisy chaining
///      to another I2C device.
///   - J6: Is a label for one VCC pin and one GND pin.  Connect MX4cK I2C
///      line's VCC and GND pins to these to power the CLS.
///   - J7: Another set of one VCC pin and one GND pin.  Use for daisy chaining
///      to another I2C device.
///////////////////////////////////////////////////////////////////////////////


// these are the I2C addresses of the warious daisy chained pmods
#define I2C_ADDR_PMOD_TEMP     0x4B
#define TWI_ADDR_PMOD_CLS      0x48
#define I2C_ADDR_PMOD_ACL      0x1D
#define I2C_ADDR_PMOD_ACL_X0   0x32
#define I2C_ADDR_PMOD_ACL_X1   0x33
#define I2C_ADDR_PMOD_ACL_Y0   0x34
#define I2C_ADDR_PMOD_ACL_Y1   0x35
#define I2C_ADDR_PMOD_ACL_Z0   0x36
#define I2C_ADDR_PMOD_ACL_Z1   0x37
#define I2C_ADDR_PMOD_ACL_PWR   0x2D
#define I2C_ADDR_PMOD_GYRO     0x69   // apparently, SDO is connected to VCC
#define I2C_ADDR_PMOD_GYRO_XL   0x28
#define I2C_ADDR_PMOD_GYRO_XH   0x29
#define I2C_ADDR_PMOD_GYRO_YL   0x2A
#define I2C_ADDR_PMOD_GYRO_YH   0x2B
#define I2C_ADDR_PMOD_GYRO_ZL   0x2C
#define I2C_ADDR_PMOD_GYRO_ZH   0x2D
#define I2C_ADDR_PMOD_GYRO_CTRL_REG1   0x20


// define the frequency (??what kind of frequency? clock frequency? bit transfer frequency? byte transfer frequency??) at which an I2C module will operate
#define I2C_FREQ_1KHZ      100000

const unsigned int I2C_TIMEOUT_MS = 5;

// Globals for setting up pmod CLS
// values in Digilent pmod CLS reference manual, pages 2 - 3
const char enable_display[] =  {27, '[', '3', 'e', '\0'};
const char set_cursor[] =     {27, '[', '1', 'c', '\0'};
const char home_cursor[] =    {27, '[', 'j', '\0'};
const char wrap_line[] =      {27, '[', '0', 'h', '\0'};
const char set_line_two[] =   {27, '[', '1', ';', '0', 'H', '\0'};


///////////////////////////////////////////////////////////////////////////////
/// @brief
///   This is the constructor for the I2C handler class.  It assigns all 
///   member variables their first values.
///
/// @return
///   None
///
/// @remark
///   Usage: Is called automatically when a new class instance is created.
///////////////////////////////////////////////////////////////////////////////
my_i2c_handler::my_i2c_handler()
{
   for (int index = 0; index < I2C_NUMBER_OF_MODULES; index += 1)
   {
      m_i2c_X_has_been_initialized[index] = false;
      m_cls_on_i2c_X_has_been_initialized[index] = false;
      m_temp_on_i2c_X_has_been_initialized[index] = false;
      m_accel_on_i2c_X_has_been_initialized[index] = false;
      m_gyro_on_i2c_X_has_been_initialized[index] = false;
   }
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///   This method returns a reference to an instance of this class.  By making 
///   class instance a static variable and the constructor private, I ensure 
///   that the only way to get at the class is through this function, and 
///   therefore only one instance of the class will ever exist at one time.
///   Also, since the variable is static and therefore independent of any 
///   instances of this class' code, the variable is created on program start, 
///   at which point the constructor is also called.
///
/// @return
///   A reference to a static variable that is an instance of this class.
///
/// @remark
///   Usage example: 
///      my_i2c_handler i2c_ref = my_i2c_handler::get_instance();
///      i2c_ref.do_one_of_the_class_methods().
///////////////////////////////////////////////////////////////////////////////
my_i2c_handler& my_i2c_handler::get_instance(void)
{
   static my_i2c_handler ref;
   
   return ref;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///   This method determines whether the I2C module specified in the argument
///   is a valid I2C module for the processor.
///
/// @remark
///   The PIC32MX460F512L only has 2 I2C channels.
///
/// @remark
///   This function in intended for the class' internal use only.  It is a 
///   support function for many other methods, and is therefore declared 
///   private.
///
/// @param[in]  module_ID
///   A value that is equivalent to one of the I2C modules specified by the 
///   typedef I2C_MODULE in peripheral/i2c.h.
///
/// @return
///   True - If the value is supported by the PIC32MX460F512L processor.
///   False - If the value is not supported by the processor.
///
/// @remark
///   Usage example:
///      if (module_is_valid(my_i2c_module_ID))
///      {
///         continue with code
///      }
///      else
///      {
///         abort code because this module doesn't exist on the processor
///      }   
///////////////////////////////////////////////////////////////////////////////
bool my_i2c_handler::module_is_valid(I2C_MODULE module_ID)
{
   if (module_ID != I2C1 && module_ID != I2C2)
   {
      // invalid module for this board; abort
      return false;
   }

   return true;
}

bool my_i2c_handler::I2C_init(I2C_MODULE module_ID, unsigned int pb_clock)
{
   // this value stores the return value of I2CSetFrequency(...), and it can
   // be used to compare the actual set frequency against the desired
   // frequency to check for discrepancies
   UINT32 actualClock;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the desired I2C line has already been initialized
   if (!m_i2c_X_has_been_initialized[module_ID])
   {
      // this I2C module has not yet been initialized, so initialize it

      // Set the I2C baudrate, then enable the module
      actualClock = I2CSetFrequency(module_ID, pb_clock, I2C_FREQ_1KHZ);
      m_pb_clock_on_i2c_X[module_ID] = pb_clock;
      I2CEnable(module_ID, TRUE);

      m_i2c_X_has_been_initialized[module_ID] = true;
   }
   else
   {
      // already initialized, so do nothing
   }

   return true;
}

bool my_i2c_handler::start_transfer(I2C_MODULE module_ID, bool start_with_restart)
{
   // thrashable storage for a return value; used in testing
   unsigned int returnVal;

   // records the status of the I2C module while waiting for it to start
   I2C_STATUS status;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   if (start_with_restart)
   {
      returnVal = I2CRepeatStart(module_ID);
   }
   else
   {
      // Wait for the bus to be idle, then start the transfer
      while(!I2CBusIsIdle(module_ID));
      returnVal = I2CStart(module_ID);
   }

   if(I2C_SUCCESS != returnVal) { return false; }

   // Wait for the signal to complete
   do
   {
      status = I2CGetStatus(module_ID);
   } while ( !(status & I2C_START) );

   return true;
}

bool my_i2c_handler::stop_transfer(I2C_MODULE module_ID)
{
   // records the status of the I2C module while waiting for it to stop
   I2C_STATUS status;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // Send the Stop signal
   I2CStop(module_ID);

   // Wait for the signal to complete
   do
   {
      status = I2CGetStatus(module_ID);
   } while ( !(status & I2C_STOP) );
}

bool my_i2c_handler::transmit_one_byte(I2C_MODULE module_ID, UINT8 data)
{
   // thrashable storage for a return value; used in testing
   unsigned int returnVal;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // Wait for the transmitter to be ready
   while(!I2CTransmitterIsReady(module_ID))
   {
   }

   // Transmit the byte
   returnVal = I2CSendByte(module_ID, data);
   if(I2C_SUCCESS != returnVal) { return false; }

   // Wait for the transmission to finish
   while(!I2CTransmissionHasCompleted(module_ID))
   {
   }

   // look for the acknowledge bit
   if(!I2CByteWasAcknowledged(module_ID)) { return false; }

   return true;
}

bool my_i2c_handler::receive_one_byte(I2C_MODULE module_ID, UINT8 *data)
{
   unsigned int timeoutStart;
   unsigned int I2CtimeoutMS = 1000;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // attempt to enable the I2C module's receiver
   /*
    * if the receiver does not enable properly, report an error, set the
    * argument to 0 (don't just leave it hanging), and return false
    */
   if(I2CReceiverEnable(module_ID, TRUE) != I2C_SUCCESS)
   {
      *data = 0;
      return false;
   }

   // wait for data to be available, then assign it when it is;
   /*
    * Note: if, prior to calling this function, the desired slave device's I2C
    * address was not sent a signal with the master's READ bit set, then the
    * desired slave device will not send data, resulting in an infinite loop
    */
   while(!(I2CReceivedDataIsAvailable(module_ID)))
   {
   }
   *data = I2CGetByte(module_ID);

   return true;
}

bool my_i2c_handler::transmit_n_bytes(I2C_MODULE module_ID, char *str, unsigned int bytesToSend)
{
   /*
    * This function performs no initialization of the I2C line or the intended
    * device.  This function is a wrapper for many transmit_one_byte(...) calls.
    */
   unsigned int byteCount;
   unsigned char c;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check that the number of bytes to send is not bogus
   if (bytesToSend > strlen(str)) { return false; }

   // initialize local variables, then send the string one byte at a time
   byteCount = 0;
   c = *str;
   while(byteCount < bytesToSend)
   {
      // transmit the bytes
      transmit_one_byte(module_ID, c);
      byteCount++;
      c = *(str + byteCount);
   }

   // transmission successful
   return true;
}

bool my_i2c_handler::CLS_write_to_line(I2C_MODULE module_ID, char* c_string, unsigned int lineNum)
{
   I2C_7_BIT_ADDRESS   slave_addr;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the CLS on this I2C line has been initialized
   if (!m_cls_on_i2c_X_has_been_initialized[module_ID]) { return false; }

   // start the I2C module and signal the CLS
   while(!start_transfer(module_ID, false))
   {
   }

   I2C_FORMAT_7_BIT_ADDRESS(slave_addr, TWI_ADDR_PMOD_CLS, I2C_WRITE);
   if (!transmit_one_byte(module_ID, slave_addr.byte)) { return false; }

   // send the cursor selection command, then send the string
   if (2 == lineNum)
   {
      if (!transmit_n_bytes(module_ID, (char*)set_line_two, strlen(set_line_two))) { return false; }
   }
   else
   {
      // not line two, so assume line 1
      if (!transmit_n_bytes(module_ID, (char*)home_cursor, strlen(home_cursor))) { return false; }
   }
   if (!transmit_n_bytes(module_ID, c_string, strlen(c_string))) { return false; }
   stop_transfer(module_ID);

   return true;
}


bool my_i2c_handler::write_device_register(I2C_MODULE module_ID, unsigned int devAddr, unsigned int regAddr, UINT8 data_byte)
{
   I2C_7_BIT_ADDRESS   slave_addr;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // send a start bit and ready the specified register on the specified device
   while(!start_transfer(module_ID, false))
   {
   }

   I2C_FORMAT_7_BIT_ADDRESS(slave_addr, devAddr, I2C_WRITE);
   if (!transmit_one_byte(module_ID, slave_addr.byte))
   {
      // stop the transmission and return false
      stop_transfer(module_ID);
      return false;
   }
   if (!transmit_one_byte(module_ID, regAddr))
   {
      // stop the transmission and return false
      stop_transfer(module_ID);
      return false;
   }
   if (!transmit_one_byte(module_ID, data_byte))
   {
      // stop the transmission and return false
      stop_transfer(module_ID);
      return false;
   }

   // stop the transmission
   stop_transfer(module_ID);

   return true;
}

bool my_i2c_handler::read_device_register(I2C_MODULE module_ID, unsigned int devAddr, unsigned int regAddr, UINT8 *data_byte)
{
   I2C_7_BIT_ADDRESS   slave_addr;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // send a start bit and ready the specified register on the specified device
   while(!start_transfer(module_ID, false))
   {
   }

   I2C_FORMAT_7_BIT_ADDRESS(slave_addr, devAddr, I2C_WRITE);
   if (!transmit_one_byte(module_ID, slave_addr.byte))
   {
      // stop the transmission and return false
      stop_transfer(module_ID);
      return false;
   }
   if (!transmit_one_byte(module_ID, regAddr))
   {
      // stop the transmission and return false
      stop_transfer(module_ID);
      return false;
   }

   // now read that register
   while(!start_transfer(module_ID, true))
   {
   }

   I2C_FORMAT_7_BIT_ADDRESS(slave_addr, devAddr, I2C_READ);
   if (!transmit_one_byte(module_ID, slave_addr.byte))
   {
      // stop the transmission and return false
      stop_transfer(module_ID);
      return false;
   }
   if (!receive_one_byte(module_ID, data_byte))
   {
      // stop the transmission and return false
      stop_transfer(module_ID);
      return false;
   }

   // stop the transmission
   stop_transfer(module_ID);

   return true;
}

bool my_i2c_handler::CLS_init(I2C_MODULE module_ID)
{
   I2C_7_BIT_ADDRESS   slave_addr;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the desired I2C line has been initialized
   if (!m_i2c_X_has_been_initialized[module_ID]) { return false; }

   // check if this CLS has already been initialized on this line
   if (m_cls_on_i2c_X_has_been_initialized[module_ID])
   {
      // already initialized, so do nothing
   }
   else
   {
      // initialized, so do your thing

      // start the I2C module, signal the CLS, and send setting strings
      while(!start_transfer(module_ID, false))
      {
      }

      I2C_FORMAT_7_BIT_ADDRESS(slave_addr, TWI_ADDR_PMOD_CLS, I2C_WRITE);
      if (!transmit_one_byte(module_ID, slave_addr.byte)) { return false; }
      if (!transmit_n_bytes(module_ID, (char*)enable_display, strlen(enable_display))) { return false; }
      if (!transmit_n_bytes(module_ID, (char*)set_cursor, strlen(set_cursor))) { return false; }
      if (!transmit_n_bytes(module_ID, (char*)home_cursor, strlen(home_cursor))) { return false; }
      if (!transmit_n_bytes(module_ID, (char*)wrap_line, strlen(wrap_line))) { return false; }
      stop_transfer(module_ID);

      m_cls_on_i2c_X_has_been_initialized[module_ID] = true;
   }

   // all went well, so return true
   return true;
}

bool my_i2c_handler::temp_init(I2C_MODULE module_ID)
{
   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the desired I2C line has been initialized
   if (!m_i2c_X_has_been_initialized[module_ID]) { return false; }

   // nothing else to do for initialization (no special register alterations
   // are needed), so turn the "initialized" flag to true
   m_temp_on_i2c_X_has_been_initialized[module_ID] = true;

   // all went well, so return true
   return true;
}

bool my_i2c_handler::accel_init(I2C_MODULE module_ID)
{
   I2C_7_BIT_ADDRESS   slave_addr;
   UINT8            data_byte;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the desired I2C line has been initialized
   if (!m_i2c_X_has_been_initialized[module_ID]) { return false; }

   // check if the accelerometer on this I2C line has already been initialized
   if (m_accel_on_i2c_X_has_been_initialized[module_ID])
   {
      // already initialized, so do nothing
   }
   else
   {
      // not yet initialized, so do your thing


      // read POWER_CTL register
      if (!read_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_PWR, &data_byte)) { return false; }

      // set standby/measure bit to measure and write it back
      data_byte |= 0x08;
      if (!write_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_PWR, data_byte)) { return false; }

      m_accel_on_i2c_X_has_been_initialized[module_ID] = true;
   }


   // all went well, so return true
   return true;
}

bool my_i2c_handler::gyro_init(I2C_MODULE module_ID)
{
   I2C_7_BIT_ADDRESS   slave_addr;
   UINT8            data_byte;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the desired I2C line has been initialized
   if (!m_i2c_X_has_been_initialized[module_ID]) { return false; }

   // check if the gyro on this I2C line has already been initialized
   if (m_gyro_on_i2c_X_has_been_initialized[module_ID])
   {
      // already initialized, so do nothing
   }
   else
   {
      // not yet initialized, so do your thing

      // read the control register that controls the power mode
      if (!read_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_CTRL_REG1, &data_byte)) { return false; }

      // set control register bit 4 from low power mode (0) to normal mode (1)
      data_byte |= 0x08;

      // write back the modified control register
      if (!write_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_CTRL_REG1, data_byte)) { return false; }

      m_gyro_on_i2c_X_has_been_initialized[module_ID] = true;
   }


   // all went well, so return true
   return true;
}


bool my_i2c_handler::temp_read(I2C_MODULE module_ID, float *fptr)
{
   I2C_7_BIT_ADDRESS   slave_addr;
   UINT8            data_byte;
   UINT32           data_uint;
   float            temperature;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the temp pmod on this I2C line has been initialized
   if (!m_temp_on_i2c_X_has_been_initialized[module_ID]) { return false; }

   // send a start signal to the I2C module
   while(!start_transfer(module_ID, false))
   {
   }

   // prepare the temperature pmod for reading
   I2C_FORMAT_7_BIT_ADDRESS(slave_addr, I2C_ADDR_PMOD_TEMP, I2C_READ);
   if (!transmit_one_byte(module_ID, slave_addr.byte)) { return false; }

   // the first read gets the high byte
   if (!receive_one_byte(module_ID, &data_byte)) { return false; }
   data_uint = data_byte << 8;

   // the second read gets the low byte
   if (!receive_one_byte(module_ID, &data_byte)) { return false; }
   data_uint |= data_byte;

   // convert the bit signal into degrees C according to the reference manual
   temperature = (data_uint >> 3) * 0.0625;

   // convert the termperature to degrees F
   temperature = ((temperature * 9) / 5) + 32;

   stop_transfer(module_ID);

   *fptr = temperature;

   return true;
}

bool my_i2c_handler::accel_read(I2C_MODULE module_ID, ACCEL_DATA *argData)
{
   INT16 localX;
   INT16 localY;
   INT16 localZ;
   UINT8 data_byte;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the accel pmod on this I2C line has been initialized
   if (!m_accel_on_i2c_X_has_been_initialized[module_ID]) { return false; }

   // read high byte of X register, then the low byte
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_X1, &data_byte)) { return false; }
   localX = data_byte << 8;

   if (!read_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_X0, &data_byte)) { return false; }
   localX |= data_byte;

   // read high byte of Y register, then the low byte
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Y1, &data_byte)) { return false; }
   localY = data_byte << 8;

   if (!read_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Y0, &data_byte)) { return false; }
   localY |= data_byte;

   // read high byte of Z register, then the low byte
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Z1, &data_byte)) { return false; }
   localZ = data_byte << 8;

   if (!read_device_register(module_ID, I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Z0, &data_byte)) { return false; }
   localZ |= data_byte;

   // all data gathered successfully, so now multiply the data by the
   // conversion factor (retrieved from Josh Sackos' PmodACL.h) and put it
   // into the argument structure
   argData->X = (float)localX * (4.0 / 1024.0);
   argData->Y = (float)localY * (4.0 / 1024.0);
   argData->Z = (float)localZ * (4.0 / 1024.0);

   return true;
}

bool my_i2c_handler::gyro_read(I2C_MODULE module_ID, GYRO_DATA *argData)
{
   I2C_7_BIT_ADDRESS   slave_addr;
   UINT8 regAddr;

   INT16 localX;
   INT16 localY;
   INT16 localZ;
   UINT8 data_byte;

   // check that we are dealing with a valid I2C module
   if (!module_is_valid(module_ID)) { return false; }

   // check if the gyro on this I2C line has been initialized
   if (!m_gyro_on_i2c_X_has_been_initialized[module_ID]) { return false; }

   // read high byte of X register, then the low byte
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_XH, &data_byte)) { return false; }
   localX = data_byte << 8;
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_XL, &data_byte)) { return false; }
   localX |= data_byte;

   // read high byte of Y register, then the low byte
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_YH, &data_byte)) { return false; }
   localY = data_byte << 8;
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_YL, &data_byte)) { return false; }
   localY |= data_byte;

   // read high byte of Z register, then the low byte
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_ZH, &data_byte)) { return false; }
   localZ = data_byte << 8;
   if (!read_device_register(module_ID, I2C_ADDR_PMOD_GYRO, I2C_ADDR_PMOD_GYRO_ZL, &data_byte)) { return false; }
   localZ |= data_byte;

   // all data gathered successfully, so now multiply the data by the
   // conversion factor (retrieved from Josh Sackos' PmodACL.h) and put it
   // into the argument structure
   argData->X = (float)localX * (500.0/65536.0);// * (4.0 / 1024.0);
   argData->Y = (float)localY * (500.0/65536.0);// * (4.0 / 1024.0);
   argData->Z = (float)localZ * (500.0/65536.0);// * (4.0 / 1024.0);

   // stop the transmission
   stop_transfer(module_ID);

   return true;
}

