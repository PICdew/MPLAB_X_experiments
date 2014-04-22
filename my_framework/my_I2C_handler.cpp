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
}

bool my_I2C_handler::init(I2C_MODULE module_ID, unsigned int pb_clock, unsigned int desired_i2c_freq)
{
   bool this_ret_val = true;

   if (!module_is_valid(module_ID))
   {
      this_ret_val = false;
   }

   if (this_ret_val)
   {
      // set the I2C baudrate, then enable the module

      // this value stores the return value of I2CSetFrequency(...), and it can
      // be used to compare the actual set frequency against the desired
      // frequency to check for discrepancies
      // Note: I don't compare it to anything, but I put it in to stop the
      // compiler from complaining.
      UINT32 actual_clock;

      actual_clock = I2CSetFrequency(module_ID, pb_clock, desired_i2c_freq);
      I2CEnable(module_ID, TRUE);

      m_I2C_has_been_initialized = true;
      m_module_ID = module_ID;

      // now set up the delay timer (assuming that it hasn't already been
      // initialized) so that future I2C waiting calls can time out instead of
      // waiting in a loop forever
      my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
      delay_timer_ref.init(pb_clock);
   }

   return this_ret_val;
}

my_I2C_handler& my_I2C_handler::get_instance(void)
{
   static my_I2C_handler ref;

   return ref;
}

bool my_I2C_handler::module_is_valid(I2C_MODULE module_ID)
{
   bool this_ret_val = true;

   if (module_ID != I2C1 && module_ID != I2C2)
   {
      this_ret_val = false;
   }

   return this_ret_val;
}


bool my_I2C_handler::start_transfer(bool start_with_restart)
{
   bool this_ret_val = true;
   unsigned int timeout_curr_time = 0;
   unsigned int timeout_start_time = 0;

   I2C_RESULT start_result;
   I2C_STATUS curr_status;

   // Wait for the bus to be idle, then start the transfer
   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
   timeout_start_time = delay_timer_ref.get_elapsed_time();
   while(!I2CBusIsIdle(m_module_ID))
   {
      timeout_curr_time = delay_timer_ref.get_elapsed_time();
      if (timeout_curr_time - timeout_start_time > I2C_TIMEOUT_MS)
      {
         // timed out, so run away
         this_ret_val = false;
         break;
      }
   }

   if (this_ret_val)
   {
      if (start_with_restart)
      {
         start_result = I2CRepeatStart(m_module_ID);
      }
      else
      {
         start_result = I2CStart(m_module_ID);
      }

      if(I2C_SUCCESS != start_result)
      {
         // oh noes! the starts has not begun to succeed!
         this_ret_val = false;

         // if there is a re-occurring master bus collision, just power cycle it
   //      if (I2C_MASTER_BUS_COLLISION == start_result)
   //      {
   //         I2C2STATCLR = _I2C2STAT_IWCOL_MASK | _I2C2STAT_BCL_MASK;
   //      }
      }
      else
      {
         // it didn't say that there was a problem, so wait for the signal to
         // complete
         timeout_start_time = delay_timer_ref.get_elapsed_time();
         do
         {
            curr_status = I2CGetStatus(m_module_ID);

            timeout_curr_time = delay_timer_ref.get_elapsed_time();
            if (timeout_curr_time - timeout_start_time > I2C_TIMEOUT_MS)
            {
               // timed out, so run away
               this_ret_val = false;
               break;
            }
         } while ( !(curr_status & I2C_START) );
      }
   }

   return this_ret_val;
}

bool my_I2C_handler::stop_transfer(void)
{
   bool this_ret_val = true;
   unsigned int timeout_curr_time = 0;
   unsigned int timeout_start_time = 0;

   // records the status of the I2C module while waiting for it to stop
   I2C_STATUS curr_status;

   // Send the Stop signal
   I2CStop(m_module_ID);

   // Wait for the signal to complete
   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
   timeout_start_time = delay_timer_ref.get_elapsed_time();
   do
   {
      curr_status = I2CGetStatus(m_module_ID);

      timeout_curr_time = delay_timer_ref.get_elapsed_time();
      if (timeout_curr_time - timeout_start_time > I2C_TIMEOUT_MS)
      {
         // timed out, so run away
         this_ret_val = false;
         break;
      }
   } while ( !(curr_status & I2C_STOP) );
   
   return this_ret_val;
}

bool my_I2C_handler::transmit_one_byte(UINT8 data)
{
   bool this_ret_val = true;
   unsigned int timeout_curr_time = 0;
   unsigned int timeout_start_time = 0;
   
   I2C_RESULT send_result;

   // Wait for the transmitter to be ready
   my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
   timeout_start_time = delay_timer_ref.get_elapsed_time();
   while(!I2CTransmitterIsReady(m_module_ID))
   {
      timeout_curr_time = delay_timer_ref.get_elapsed_time();
      if (timeout_curr_time - timeout_start_time > I2C_TIMEOUT_MS)
      {
         // timed out, so run away
         this_ret_val = false;
         break;
      }
   }

   if (this_ret_val)
   {
      // transmitter is read, so transmit the byte
      send_result = I2CSendByte(m_module_ID, data);
      if(I2C_SUCCESS != send_result)
      {
         this_ret_val = false;
      }
   }
   
   if (this_ret_val)
   {
      // the transmission started ok, so wait for the transmission to finish
      timeout_start_time = delay_timer_ref.get_elapsed_time();
      while(!I2CTransmissionHasCompleted(m_module_ID))
      {
         timeout_curr_time = delay_timer_ref.get_elapsed_time();
         if (timeout_curr_time - timeout_start_time > I2C_TIMEOUT_MS)
         {
            // timed out, so run away
            this_ret_val = false;
            break;
         }
      }
   }

   if (this_ret_val)
   {
      // transmission finished; look for the acknowledge bit
      if(!I2CByteWasAcknowledged(m_module_ID)) 
      { 
         this_ret_val = false;
      }
   }

   return this_ret_val;
}


bool my_I2C_handler::transmit_n_bytes(const char *str, unsigned int bytes_to_send)
{
   // this function performs no initialization of the I2C line or the intended
   // device; it is a wrapper for many TransmitOneByte(...) calls
   bool this_ret_val = true;
   bool send_success = true;
   unsigned int byteCount;
   unsigned char c;

   // check that the number of bytes to send is not bogus
   if (bytes_to_send > strlen(str))
   { 
      this_ret_val = false;
   }

   if (this_ret_val)
   {
      // initialize local variables, then send the string one byte at a time
      byteCount = 0;
      c = *str;
      while(byteCount < bytes_to_send)
      {
         send_success = transmit_one_byte(c);
         
         if (send_success)
         {
            byteCount++;
            c = *(str + byteCount);
         }
         else
         {
            // uh oh; send failed
            this_ret_val = false;
            break;
         }
      }
   }

   return this_ret_val;
}


bool my_I2C_handler::receive_one_byte(UINT8 *data_byte_ptr)
{
   bool this_ret_val = true;
   unsigned int timeout_curr_time = 0;
   unsigned int timeout_start_time = 0;

   // attempt to enable the I2C module's receiver
   /*
   * if the receiver does not enable properly, report an error, set the
   * argument to 0 (don't just leave it hanging), and return false
   */
   if(I2C_SUCCESS != I2CReceiverEnable(m_module_ID, TRUE))
   {
      *data_byte_ptr = 0;
      this_ret_val = false;
   }

   if (this_ret_val)
   {
      // wait for data to be available, then assign it when it is
      // Note: If, prior to calling this function, the desired slave device's
      // I2C address was not sent a signal with the master's READ bit set, then
      // the desired slave device will not send data, resulting in an infinite
      // loop.
      my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
      timeout_start_time = delay_timer_ref.get_elapsed_time();
      while(!I2CReceivedDataIsAvailable(m_module_ID))
      {
         timeout_curr_time = delay_timer_ref.get_elapsed_time();
         if (timeout_curr_time - timeout_start_time > I2C_TIMEOUT_MS)
         {
            // timed out, so run away
            this_ret_val = false;
            break;
         }
      }
      *data_byte_ptr = I2CGetByte(m_module_ID);
   }

   return this_ret_val;
}

bool my_I2C_handler::write_device_register(unsigned int dev_addr, unsigned int reg_addr, UINT8 data_byte)
{
   bool this_ret_val = true;
   I2C_7_BIT_ADDRESS slave_address;

   // send a start bit and ready the specified register on the specified device
   if(!start_transfer(false))
   {
      this_ret_val = false;
   }

   if (this_ret_val)
   {
      I2C_FORMAT_7_BIT_ADDRESS(slave_address, dev_addr, I2C_WRITE);
      if (!transmit_one_byte(slave_address.byte))
      {
         this_ret_val = false;
      }
   }

   if (this_ret_val)
   {
      if (!transmit_one_byte(reg_addr))
      {
         this_ret_val = false;
      }
   }

   if (this_ret_val)
   {
      if (!transmit_one_byte(data_byte))
      {
         this_ret_val = false;
      }
   }

   // stop the transmission
   stop_transfer();


   return this_ret_val;
}

bool my_I2C_handler::read_device_register(unsigned int dev_addr, unsigned int reg_addr, UINT8 *data_byte_ptr)
{
   bool this_ret_val = false;
   I2C_7_BIT_ADDRESS slave_address;

   // send a start bit and ready the specified register on the specified device
   if(!start_transfer(false))
   {
      this_ret_val = false;
   }

   if (this_ret_val)
   {
      I2C_FORMAT_7_BIT_ADDRESS(slave_address, dev_addr, I2C_WRITE);
      if (!transmit_one_byte(slave_address.byte))
      {
         this_ret_val = false;
      }
   }

   if (this_ret_val)
   {
      if (!transmit_one_byte(reg_addr))
      {
         this_ret_val = false;
      }
   }

   // if everything has gone ok so far, try to read that register
   if (this_ret_val)
   {
      if (!start_transfer(true))
      {
         this_ret_val = false;
      }
   }

   if (this_ret_val)
   {
      I2C_FORMAT_7_BIT_ADDRESS(slave_address, dev_addr, I2C_READ);
      if (!transmit_one_byte(slave_address.byte))
      {
         this_ret_val = false;
      }
   }

   if (this_ret_val)
   {
      if (!receive_one_byte(data_byte_ptr))
      {
         this_ret_val = false;
      }
   }

   // stop the transmission
   stop_transfer();

   return this_ret_val;
}


bool my_I2C_handler::CLS_init(void)
{
   bool this_ret_val = true;
   unsigned int timeout_curr_time = 0;
   unsigned int timeout_start_time = 0;

   if (!m_I2C_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // I2C module has been initialized, so proceed with CLS initialization

      // start the I2C module, signal the CLS, and send setting strings
      my_delay_timer delay_timer_ref = my_delay_timer::get_instance();
      timeout_start_time = delay_timer_ref.get_elapsed_time();
      if(!start_transfer(false))
      {
         this_ret_val = false;
      }

      if (this_ret_val)
      {
         I2C_7_BIT_ADDRESS slave_address;
         I2C_FORMAT_7_BIT_ADDRESS(slave_address, TWI_ADDR_PMOD_CLS, I2C_WRITE);
         if (!transmit_one_byte(slave_address.byte))
         {
            this_ret_val = false;
         }

         if (this_ret_val)
         {
            if (!transmit_n_bytes((char*)enable_display, strlen(enable_display)))
            {
               this_ret_val = false;
            }
         }

         if (this_ret_val)
         {
            if (!transmit_n_bytes((char*)set_cursor, strlen(set_cursor)))
            {
               this_ret_val = false;
            }
         }

         if (this_ret_val)
         {
            if (!transmit_n_bytes((char*)set_line_one, strlen(set_line_one)))
            {
               this_ret_val = false;
            }
         }

         if (this_ret_val)
         {
            if (!transmit_n_bytes((char*)wrap_line, strlen(wrap_line)))
            {
               this_ret_val = false;
            }
         }

         stop_transfer();
      }
   }

   if (this_ret_val)
   {
      // nothing bad happened, so initialization was a success
      m_CLS_has_been_initialized = true;
   }

   return this_ret_val;
}

bool my_I2C_handler::CLS_write_to_line(const char* string, unsigned int lineNum)
{
   bool this_ret_val = true;

   if (!m_CLS_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // has been initialized

      // start the I2C module and signal the CLS
      if (!start_transfer(false))
      {
         this_ret_val = false;
      }

      if (this_ret_val)
      {
         I2C_7_BIT_ADDRESS slave_address;
         I2C_FORMAT_7_BIT_ADDRESS(slave_address, TWI_ADDR_PMOD_CLS, I2C_WRITE);
         if (!transmit_one_byte(slave_address.byte))
         {
            this_ret_val = false;
         }

         if (this_ret_val)
         {
            // send the cursor selection command, then send the string
            if (2 == lineNum)
            {
               if (!transmit_n_bytes((char *)set_line_two, strlen(set_line_two)))
               {
                  this_ret_val = false;
               }
            }
            else
            {
               // not line two, so assume line 1 (don't throw a fit or anything if
               // it isn't 1)
               if (!transmit_n_bytes((char *)set_line_one, strlen(set_line_one)))
               {
                  this_ret_val = false;
               }
            }

            if (this_ret_val)
            {
               if (!transmit_n_bytes(string, strlen(string)))
               {
                  this_ret_val = false;
               }
            }
         }

         stop_transfer();
      }
   }

   return this_ret_val;
}


bool my_I2C_handler::temp_init(void)
{
   // nothing much to do except ceck that the I2C module itself has already
   // been initialized, but if something else needs to be done in the future,
   // this is where pmod TMP init stuff will go

   if (m_I2C_has_been_initialized)
   {
      m_TMP_has_been_initialized = true;
   }

   return true;
}

bool my_I2C_handler::temp_read(float *f_ptr, bool read_in_F)
{
   bool this_ret_val = true;
   I2C_7_BIT_ADDRESS   slave_address;
   UINT8               data_byte = 0;
   UINT32              data_uint = 0;
   float               temperature = 0.0f;

   if (!m_TMP_has_been_initialized)
   {
      this_ret_val = false;
   }

   if (this_ret_val)
   {
      if (!start_transfer(false))
      {
         this_ret_val = false;
      }
   }

   if (this_ret_val)
   {
      I2C_FORMAT_7_BIT_ADDRESS(slave_address, I2C_ADDR_PMOD_TEMP, I2C_READ);
      if (!transmit_one_byte(slave_address.byte))
      {
         this_ret_val = false;
      }
   }

   // you can only read one byte at a time, but the temperature pmod has 16bits
   // of data that are intended for a float, so do two reads
   // Note: The first read reads the high byte, and the second read without
   // stopping the transfer will read the low byte.  If you tried to read
   // another byte after that, I don't know what would happen.
   if (this_ret_val)
   {
      if (!receive_one_byte(&data_byte))
      {
         this_ret_val = false;
      }
      else
      {
         data_uint = data_byte << 8;
      }
   }

   if (this_ret_val)
   {
      if (!receive_one_byte(&data_byte))
      {
         this_ret_val = false;
      }
      else
      {
         data_uint |= data_byte;
      }
   }

   if (this_ret_val)
   {
      // all the readings seemed to go okay, so convert the bit signal into
      // degrees Celsius according to the reference manual
      temperature = (data_uint >> 3) * 0.0625;

      if (read_in_F)
      {
         temperature = ((temperature * 9) / 5) + 32;
      }
   }

   *f_ptr = temperature;
   stop_transfer();

   return this_ret_val;
}

