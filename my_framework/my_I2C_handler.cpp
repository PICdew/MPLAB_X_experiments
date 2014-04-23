// for linking these definitions with their declarations
#include "my_I2C_handler.h"

// for allowing timeouts instead of having I2C hangups perpetually hang up the 
// whole program if there is a bad start or the bus fails to go idle or 
// some device gets unplugged
#include "my_delay_timer.h"

extern "C"
{
#include <peripheral/system.h>
#include <peripheral/i2c.h>
}


// these I2C address are unique to the pmod hardware (see ref manuals for them)
// and are independent of any program that uses them, so it is ok for them to
// be hidden in this source file
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

// for timeouts so that while(...) loops don't hang forever
const unsigned int I2C_TIMEOUT_MS = 10;

// Globals for setting up pmod CLS
// values in Digilent pmod CLS reference manual, pages 2 - 3
static const char enable_display[] =  {27, '[', '3', 'e', '\0'};
static const char set_cursor[] =      {27, '[', '1', 'c', '\0'};
static const char set_line_one[] =    {27, '[', 'j', '\0'};
static const char wrap_line[] =       {27, '[', '0', 'h', '\0'};
static const char set_line_two[] =    {27, '[', '1', ';', '0', 'H', '\0'};


my_I2C_handler::my_I2C_handler()
{
   m_I2C_has_been_initialized = false;
   m_CLS_has_been_initialized = false;
   m_TMP_has_been_initialized = false;
   m_ACL_has_been_initialized = false;
}

int my_I2C_handler::init(I2C_MODULE module_ID, unsigned int pb_clock, unsigned int desired_i2c_freq)
{
   if (!module_is_valid(module_ID)) { return -1; }

   // set the I2C baudrate, then enable the module
   UINT32 actual_clock;
   actual_clock = I2CSetFrequency(module_ID, pb_clock, desired_i2c_freq);
   I2CEnable(module_ID, TRUE);

   m_I2C_has_been_initialized = true;
   m_module_ID = module_ID;

   // call this AFTER the member module ID has been assigned
   stop_transfer();

   return 0;
}

my_I2C_handler& my_I2C_handler::get_instance(void)
{
   static my_I2C_handler ref;

   return ref;
}

int my_I2C_handler::module_is_valid(I2C_MODULE module_ID)
{
   int err_code = -1;

   if (module_ID != I2C1 && module_ID != I2C2) { return -1; }

   return 0;
}


int my_I2C_handler::start_transfer(bool start_with_restart)
{
   int err_code = -1;

   I2C_RESULT start_result;
   I2C_STATUS curr_status;

   if (start_with_restart)
   {
      // restart the bus; don't wait for it to be idle
      start_result = I2CRepeatStart(m_module_ID);
   }
   else
   {
      // Wait for the bus to be idle, then start the transfer
      while(!I2CBusIsIdle(m_module_ID));
      start_result = I2CStart(m_module_ID);
   }

   if(I2C_SUCCESS != start_result) { return -1; }

   // it didn't say that there was a problem, so wait for the signal to
   // complete
   do
   {
      curr_status = I2CGetStatus(m_module_ID);
   } while ( !(curr_status & I2C_START) );

   return 0;
}

int my_I2C_handler::stop_transfer(void)
{
   int err_code = -1;

   // records the status of the I2C module while waiting for it to stop
   I2C_STATUS curr_status;

   // Send the Stop signal
   I2CStop(m_module_ID);

   // Wait for the signal to complete
   do
   {
      curr_status = I2CGetStatus(m_module_ID);
   } while ( !(curr_status & I2C_STOP) );
   
   return 0;
}

int my_I2C_handler::transmit_one_byte(UINT8 data)
{
   int err_code = -1;
   
   I2C_RESULT send_result;

   // Wait for the transmitter to be ready
   while(!I2CTransmitterIsReady(m_module_ID));

   // transmitter is read, so transmit the byte
   send_result = I2CSendByte(m_module_ID, data);
   if(I2C_SUCCESS != send_result) { return -1; }
   
   // the transmission started ok, so wait for the transmission to finish
   while(!I2CTransmissionHasCompleted(m_module_ID));

   // transmission finished; look for the acknowledge bit
   if(!I2CByteWasAcknowledged(m_module_ID)) { return -2; }

   return 0;
}


int my_I2C_handler::transmit_n_bytes(const char *str, unsigned int bytes_to_send)
{
   // this function performs no initialization of the I2C line or the intended
   // device; it is a wrapper for many TransmitOneByte(...) calls
   int err_code = -1;
   bool send_success = true;
   unsigned int byte_count;
   unsigned char c;

   // check that the number of bytes to send is not bogus
   if (bytes_to_send > strlen(str)) { return -1; }

   // initialize local variables, then send the string one byte at a time
   byte_count = 0;
   c = *str;
   while(byte_count < bytes_to_send)
   {
      send_success = transmit_one_byte(c);
      byte_count++;
      c = *(str + byte_count);
   }

   return 0;
}


int my_I2C_handler::receive_one_byte(UINT8 *data_byte_ptr)
{
   int err_code = -1;

   if(I2C_SUCCESS != I2CReceiverEnable(m_module_ID, TRUE)) 
   {
      *data_byte_ptr = 0;
      return -1;
   }

   while(!I2CReceivedDataIsAvailable(m_module_ID));
   *data_byte_ptr = I2CGetByte(m_module_ID);

   return 0;
}

int my_I2C_handler::write_device_register(unsigned int dev_addr, unsigned int reg_addr, UINT8 data_byte)
{
   int err_code = -1;
   I2C_7_BIT_ADDRESS slave_address;

   // send a start bit and ready the specified register on the specified device
   while(!start_transfer(false));

   I2C_FORMAT_7_BIT_ADDRESS(slave_address, dev_addr, I2C_WRITE);
   if (!transmit_one_byte(slave_address.byte)) 
   {
      stop_transfer();
      return -2;
   }

   if (!transmit_one_byte(reg_addr)) 
   {
      stop_transfer();
      return -3;
   }

   if (!transmit_one_byte(data_byte))
   {
      stop_transfer();
      return -4;
   }

   // stop the transmission
   stop_transfer();

   return 0;
}

int my_I2C_handler::read_device_register(unsigned int dev_addr, unsigned int reg_addr, UINT8 *data_byte_ptr)
{
   int err_code = -1;
   I2C_7_BIT_ADDRESS slave_address;

   // send a start bit and ready the specified register on the specified device
   while(!start_transfer(false));
   I2C_FORMAT_7_BIT_ADDRESS(slave_address, dev_addr, I2C_WRITE);
   if (!transmit_one_byte(slave_address.byte))
   {
      stop_transfer();
      return -1;
   }

   if (!transmit_one_byte(reg_addr))
   { 
      stop_transfer();
      return -2;
   }

   // restart the transmission for reading
   while(!start_transfer(true));
   I2C_FORMAT_7_BIT_ADDRESS(slave_address, dev_addr, I2C_READ);
   if (!transmit_one_byte(slave_address.byte))
   { 
      stop_transfer();
      return err_code;
   }

   if (!receive_one_byte(data_byte_ptr))
   {
      stop_transfer();
      return err_code;
   }

   // stop the transmission
   stop_transfer();

   return 0;
}


int my_I2C_handler::CLS_init(void)
{
   int err_code = -1;
   I2C_7_BIT_ADDRESS slave_address;

   if (!m_I2C_has_been_initialized) { return -1; }

   // start the I2C module, signal the CLS, and send setting strings
   while(!start_transfer(false));
   I2C_FORMAT_7_BIT_ADDRESS(slave_address, TWI_ADDR_PMOD_CLS, I2C_WRITE);
   if (!transmit_one_byte(slave_address.byte)) { return -2; }
   if (!transmit_n_bytes((char*)enable_display, strlen(enable_display))) { return -3; }
   if (!transmit_n_bytes((char*)set_cursor, strlen(set_cursor))) { return -4; }
   if (!transmit_n_bytes((char*)set_line_one, strlen(set_line_one))) { return -5; }
   if (!transmit_n_bytes((char*)wrap_line, strlen(wrap_line))) { return -6; }
   stop_transfer();

   // nothing bad happened, so initialization was a success
   m_CLS_has_been_initialized = true;

   return 0;
}

int my_I2C_handler::CLS_write_to_line(const char* str, unsigned int lineNum)
{
   int err_code = -1;

   if (!m_CLS_has_been_initialized)
   { return err_code; } else { err_code -= 1; }

   // has been initialized

   // start the transfer and signal the CLS
   while(!start_transfer(false));

   I2C_7_BIT_ADDRESS slave_address;
   I2C_FORMAT_7_BIT_ADDRESS(slave_address, TWI_ADDR_PMOD_CLS, I2C_WRITE);
   if (!transmit_one_byte(slave_address.byte)) { return -1; }

   // send the cursor selection command, then send the string
   if (2 == lineNum)
   {
      if (!transmit_n_bytes((char *)set_line_two, strlen(set_line_two))) { return -2; }
   }
   else
   {
      // not line two, so assume line 1 (don't throw a fit or anything if
      // it isn't 1)
      if (!transmit_n_bytes((char *)set_line_one, strlen(set_line_one))) { return -3; }
   }

   if (!transmit_n_bytes(str, strlen(str))) { return -4; }

   stop_transfer();

   return 0;
}


int my_I2C_handler::temp_init(void)
{
   // nothing much to do except ceck that the I2C module itself has already
   // been initialized, but if something else needs to be done in the future,
   // this is where pmod TMP init stuff will go

   if (m_I2C_has_been_initialized)
   {
      m_TMP_has_been_initialized = true;
   }

   return 0;
}

int my_I2C_handler::temp_read(float *f_ptr, bool read_in_F)
{
   int err_code = -1;
   I2C_7_BIT_ADDRESS   slave_address;
   UINT8               data_byte = 0;
   UINT32              data_uint = 0;
   float               temperature = 0.0f;

   if (!m_TMP_has_been_initialized) { return -1; }

   while(!start_transfer(false));

   I2C_FORMAT_7_BIT_ADDRESS(slave_address, I2C_ADDR_PMOD_TEMP, I2C_READ);
   if (!transmit_one_byte(slave_address.byte)) { return -2; }

   if (!receive_one_byte(&data_byte)) { return -3; }
   data_uint = data_byte << 8;

   if (!receive_one_byte(&data_byte)) { return -4; }
   data_uint |= data_byte;

   // all the readings seemed to go okay, so convert the bit signal into
   // degrees Celsius according to the reference manual
   temperature = (data_uint >> 3) * 0.0625;

   if (read_in_F)
   {
      temperature = ((temperature * 9) / 5) + 32;
   }

   *f_ptr = temperature;
   stop_transfer();

   return 0;
}


int my_I2C_handler::acl_init(void)
{
   int err_code = -1;
   I2C_7_BIT_ADDRESS slave_address;
   UINT8 data_byte;

   while(!start_transfer(false));
   I2C_FORMAT_7_BIT_ADDRESS(slave_address, I2C_ADDR_PMOD_ACL, I2C_WRITE);
   if (!transmit_one_byte(slave_address.byte)) { return -1; }
   if (!transmit_one_byte(I2C_ADDR_PMOD_ACL_PWR)) { return -2; }
   while(!start_transfer(true));
   I2C_FORMAT_7_BIT_ADDRESS(slave_address, I2C_ADDR_PMOD_ACL, I2C_READ);
   if (!transmit_one_byte(slave_address.byte)) { return -3; }
   if (!receive_one_byte(&data_byte)) { return -4; }

   data_byte |= 0x08;

   while(!start_transfer(true));
   I2C_FORMAT_7_BIT_ADDRESS(slave_address, I2C_ADDR_PMOD_ACL, I2C_WRITE);
   if (!transmit_one_byte(slave_address.byte)) { return -5; }
   if (!transmit_one_byte(I2C_ADDR_PMOD_ACL_PWR)) { return -6; }
   if (!transmit_one_byte(data_byte)) { return -7; }

   stop_transfer();

   m_ACL_has_been_initialized = true;

   return 0;
}

int my_I2C_handler::acl_read(ACCEL_DATA *data_ptr)
{
   int err_code = -1;
   INT16 local_X = 0;
   INT16 local_Y = 0;
   INT16 local_Z = 0;
   UINT8 data_byte = 0;

   if (!m_ACL_has_been_initialized) { return -1; }
   if (0 == data_ptr) { return -2; }

   if (!read_device_register(I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_X1, &data_byte)) { return -3; }
   local_X = data_byte << 8;
   if (!read_device_register(I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_X0, &data_byte)) { return -4; }
   local_X |= data_byte;

   if (!read_device_register(I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Y1, &data_byte)) { return -5; }
   local_Y = data_byte << 8;
   if (!read_device_register(I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Y0, &data_byte)) { return -6; }
   local_Y |= data_byte;

   if (!read_device_register(I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Z1, &data_byte)) { return -7; }
   local_Z = data_byte << 8;
   if (!read_device_register(I2C_ADDR_PMOD_ACL, I2C_ADDR_PMOD_ACL_Z0, &data_byte)) { return -8; }
   local_Z |= data_byte;

   // all data gathered successfully, so now multiply the raw bits by the
   // conversion factor so that it is valid floating data
   // (??how was this done again??)
   data_ptr->X = (float)local_X * (4.0f / 1024.0f);
   data_ptr->Y = (float)local_Y * (4.0f / 1024.0f);
   data_ptr->Z = (float)local_Z * (4.0f / 1024.0f);

   return 0;
}
