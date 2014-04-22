// for linking these definitions with their declarations
#include "my_I2C_handler.h"

#include "../my_framework/WOOP_WOOP_WOOP.h"


#ifdef __cplusplus
extern "C" {
#endif

#include <peripheral/i2c.h>

#ifdef __cplusplus
}
#endif


// this address is unique to the CLS pmod (see CLS ref manual)
#define TWI_ADDR_PMOD_CLS        0x48

// ??where do I find valid limits of I2C frequencies??
#define DESIRED_I2C_FREQ_1KHZ    100000

// for the CLS; used when formating strings to fit in a line
#define CLS_LINE_SIZE      17



// Globals for setting up pmod CLS
// values in Digilent pmod CLS reference manual, pages 2 - 3
static const char enable_display[] =  {27, '[', '3', 'e', '\0'};
static const char set_cursor[] =      {27, '[', '1', 'c', '\0'};
static const char home_cursor[] =     {27, '[', 'j', '\0'};
static const char wrap_line[] =       {27, '[', '0', 'h', '\0'};
static const char set_line_two[] =    {27, '[', '1', ';', '0', 'H', '\0'};


my_I2C_handler::my_I2C_handler()
{
   m_has_been_initialized = false;
}

bool my_I2C_handler::init_I2C(I2C_MODULE module_ID, unsigned int sys_clock, unsigned int desired_i2c_freq)
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

      actual_clock = I2CSetFrequency(module_ID, sys_clock, desired_i2c_freq);
      I2CEnable(module_ID, TRUE);

      m_has_been_initialized = true;
      m_module_ID = module_ID;
   }

   return this_ret_val;
}

static my_I2C_handler::my_I2C_handler& get_instance(void)
{
   static my_I2C_handler ref();

   return ref;
}

bool my_I2C_handler::module_is_valid(I2C_MODULE module_ID)
{
   bool this_ret_val = true;

   if (!m_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // has been initialized

      if (module_ID != I2C1 && module_ID != I2C2)
      {
         this_ret_val = false;
      }
   }

   return this_ret_val;
}


bool my_I2C_handler::start_transfer_without_restart(void)
{
   bool this_ret_val = true;
   
   I2C_RESULT start_result;
   I2C_STATUS curr_status;

   if (!m_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // has been initialized

      // Wait for the bus to be idle, then start the transfer
      while(!I2CBusIsIdle(m_module_ID));

      start_result = I2CStart(m_module_ID);
      if(I2C_SUCCESS != start_result)
      {
         // oh noes! the starts has not begun to succeed!
         this_ret_val = false;
      }
      else
      {
         // it didn't say that there was a problem, so wait for the signal to
         // complete
         do
         {
            curr_status = I2CGetStatus(m_module_ID);

         } while ( !(curr_status & I2C_START) );
      }
   }

   return this_ret_val;
}

bool my_I2C_handler::start_transfer_with_restart(void)
{
   bool this_ret_val = true;
   
   I2C_RESULT start_result;
   I2C_STATUS curr_status;

   if (!m_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // has been initialized

      // Wait for the bus to be idle, then start the transfer
      while(!I2CBusIsIdle(m_module_ID));

      start_result = I2CRepeatStart(m_module_ID);
      if(I2C_SUCCESS != start_result)
      {
         // oh noes! the starts has not begun to succeed!
         this_ret_val = false;
      }
      else
      {
         // it didn't say that there was a problem, so wait for the signal to
         // complete
         do
         {
            curr_status = I2CGetStatus(m_module_ID);

         } while ( !(curr_status & I2C_START) );
      }
   }
   
   return this_ret_val;
}

bool my_I2C_handler::stop_transfer(void)
{
   bool this_ret_val = true;

   if (!m_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // has been initialized

      // records the status of the I2C module while waiting for it to stop
      I2C_STATUS curr_status;

      // Send the Stop signal
      I2CStop(m_module_ID);

      // Wait for the signal to complete
      do
      {
         curr_status = I2CGetStatus(m_module_ID);

      } while ( !(curr_status & I2C_STOP) );
   }

   
   return this_ret_val;
}

bool my_I2C_handler::transmit_one_byte(UINT8 data)
{
   bool this_ret_val = true;
   
   I2C_RESULT send_result;

   // Wait for the transmitter to be ready
   while(!I2CTransmitterIsReady(m_module_ID));

   // Transmit the byte
   send_result = I2CSendByte(m_module_ID, data);
   if(I2C_SUCCESS != returnVal) 
   { 
      this_ret_val = false;
   }
   
   if (this_ret_val)
   {
      // Wait for the transmission to finish
      while(!I2CTransmissionHasCompleted(m_module_ID));
      
      // look for the acknowledge bit
      if(!I2CByteWasAcknowledged(m_module_ID)) 
      { 
         this_ret_val = false;
      }
   }

   return this_ret_val;
}


bool transmit_n_bytes(const char *str, unsigned int bytes_to_send)
{
   // this function performs no initialization of the I2C line or the intended
   // device; it is a wrapper for many TransmitOneByte(...) calls
   bool this_ret_val = true;
   bool send_success = true;
   unsigned int byteCount;
   unsigned char c;

   // check that the number of bytes to send is not bogus
   if (bytesToSend > strlen(str)) 
   { 
      this_ret_val = false;
   }

   if (this_ret_val)
   {
      // initialize local variables, then send the string one byte at a time
      byteCount = 0;
      c = *str;
      while(byteCount < bytesToSend)
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


bool my_I2C_handler::my_I2C_CLS_init(void)
{
   bool this_ret_val = true;

   if (!m_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // has been initialized

      // start the I2C module, signal the CLS, and send setting strings
      while(!start_transfer_without_restart(m_module_ID));
      I2C_7_BIT_ADDRESS SlaveAddress;
      I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, TWI_ADDR_PMOD_CLS, I2C_WRITE);
      if (!transmit_one_byte(m_module_ID, SlaveAddress.byte))
      {
         this_ret_val = false;
      }

      if (this_ret_val)
      {
         if (!transmit_n_bytes(m_module_ID, (char*)enable_display, strlen(enable_display)))
         {
            this_ret_val = false;
         }
      }

      if (this_ret_val)
      {
         if (!transmit_n_bytes(m_module_ID, (char*)set_cursor, strlen(set_cursor)))
         {
            this_ret_val = false;
         }
      }

      if (this_ret_val)
      {
         if (!transmit_n_bytes(m_module_ID, (char*)home_cursor, strlen(home_cursor)))
         {
            this_ret_val = false;
         }
      }

      if (this_ret_val)
      {
         if (!transmit_n_bytes(m_module_ID, (char*)wrap_line, strlen(wrap_line)))
         {
            this_ret_val = false;
         }
      }

      stop_transfer(m_module_ID);
   }

   return this_ret_val;
}

bool my_I2C_handler::write_to_line(const char* string, unsigned int lineNum)
{
   bool this_ret_val = true;

   if (!m_has_been_initialized)
   {
      this_ret_val = false;
   }
   else
   {
      // has been initialized

      // start the I2C module and signal the CLS
      while(!start_transfer_without_restart(m_module_ID));
      I2C_7_BIT_ADDRESS SlaveAddress;
      I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, TWI_ADDR_PMOD_CLS, I2C_WRITE);
      if (!transmit_one_byte(m_module_ID, SlaveAddress.byte))
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
            // not line two, so assume line 1 (don't throw a fit or anything)
            if (!transmit_n_bytes((char *)home_cursor, strlen(home_cursor)))
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

      stop_transfer(m_module_ID);
   }

   return this_ret_val;
}
