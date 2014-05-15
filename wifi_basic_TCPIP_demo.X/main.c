#include <peripheral/system.h>
#include <peripheral/timer.h>
#include <peripheral/ports.h>
#include <peripheral/i2c.h>

#include <stdio.h>

#include "my_TCPIP_framework.h"

#include "../../github_my_MX4cK_framework/my_C_I2C_handler.h"
#include "../../github_my_MX4cK_framework/my_function_queue.h"



// ------------------ Configuration Oscillators --------------------------------
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK  = 40 MHz
// -----------------------------------------------------------------------------
#pragma config FPLLMUL = MUL_20
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLODIV = DIV_1
#pragma config FWDTEN = OFF
#pragma config POSCMOD = HS
#pragma config FNOSC = PRIPLL
#pragma config FPBDIV = DIV_2
//#pragma config CP = OFF, BWP = OFF, PWP = OFF
//#pragma config UPLLEN = OFF


#define SYSTEM_CLOCK          80000000
#define PB_DIV                2
#define T1_PS                 64
#define T1_TOGGLES_PER_SEC    1000
#define T1_TICK_PR            SYSTEM_CLOCK/PB_DIV/T1_PS/T1_TOGGLES_PER_SEC
#define T1_OPEN_CONFIG        T1_ON | T1_SOURCE_INT | T1_PS_1_64

unsigned int gMillisecondsInOperation;
BOOL g_TCPIP_service_can_start;

void TCPIP_send_receive(void);

void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
   gMillisecondsInOperation++;

   if (0 == gMillisecondsInOperation % 50)
   {
      PORTToggleBits(IOPORT_B, BIT_11);

      if (g_TCPIP_service_can_start)
      {
         // service the TCPIP stack
         add_function_to_queue(TCPIP_keep_stack_alive);
      }
   }

   // clear the interrupt flag
   mT1ClearIntFlag();
}

void delayMS(unsigned int milliseconds)
{
   unsigned int millisecondCount = gMillisecondsInOperation;
   while ((gMillisecondsInOperation - millisecondCount) < milliseconds);
}

// this is the port number that we will be using
// Note: This can be any number, really, that is within the range of a 16 bit
// integer because the TCPIP stack that this program is using uses a WORD for
// the port type, which is a typedef of an unsigned short int.  Just make sure
// that your Tera Term connection also specifies the same port.
#define SERVER_PORT	5

// this is the size of our message buffer that we will use to receive data from
// and send data over the network via TCPIP
// Note: This is a #define so that it can be used in both the buffer
// declaration and when a maximum buffer size needs to be specified.
#define MESSAGE_BUFFER_SIZE_BYTES 64

void blink_onboard_LED_1(void)
{
   if (0 == (gMillisecondsInOperation % 50))
   {
      PORTToggleBits(IOPORT_B, BIT_10);
   }
}

void my_TCPIP_receive_and_reply(void)
{
   int byte_count = 0;
   char cls_message[CLS_LINE_SIZE + 1];
   unsigned char rx_buffer[MESSAGE_BUFFER_SIZE_BYTES];
   unsigned char tx_buffer[MESSAGE_BUFFER_SIZE_BYTES];

   // I like to blink different LEDs depending on whether there is a
   // connection or not, and I can get away with it because I am only using
   // a single socket-port
   // Note: Blinking LEDs would be a poor way to check the connection status
   // of a multiple socket-port setup.
   if (0 == (gMillisecondsInOperation % 50))
   {
      if (1 == TCPIP_is_there_a_connection_on_port(SERVER_PORT))
      {
         PORTClearBits(IOPORT_B, BIT_12);
         PORTToggleBits(IOPORT_B, BIT_13);
      }
      else
      {
         PORTClearBits(IOPORT_B, BIT_13);
         PORTToggleBits(IOPORT_B, BIT_12);
      }
   }

   byte_count = TCPIP_bytes_in_RX_FIFO(SERVER_PORT);
   if (byte_count > 0)
   {
      myI2CWriteToLine(I2C2, "data Data DATA!", 1);
      myI2CWriteToLine(I2C2, "bats Bats BATS!", 2);

      // missige for you, sire
      snprintf(cls_message, CLS_LINE_SIZE, "RX bytes: %d", byte_count);
      myI2CWriteToLine(I2C2, cls_message, 1);

      // clear out the message buffer, get the missige, then display it on
      // the CLS pmod, line 2 (because I want "missige sire:" to display on
      // the top line)
      // Note: If the message is longer than a single CLS line, that is ok.
      // It will just start overwriting the last character until the string
      // is finished.
      memset(rx_buffer, 0, MESSAGE_BUFFER_SIZE_BYTES);
      byte_count = TCPIP_basic_receive(SERVER_PORT, rx_buffer, MESSAGE_BUFFER_SIZE_BYTES);
      if (byte_count > CLS_LINE_SIZE)
      {
         // ??do something if it is too long to print on a single CLS line? no? do we care??
      }
      else if (byte_count == MESSAGE_BUFFER_SIZE_BYTES)
      {
         // do something?
      }

      // forcefully null terminate the receive buffer to make sure that
      // formated string (%s) procede without incident
      // Note: Chop off the last two bytes, which are the return carriage
      // and new line characters that Tera Term sends when the return key is
      // pressed.
      // Note: If not using Tera Term, do not chop off the last two bytes
      // unless you know that the sending program appends unwanted
      // characters or other bytes to the end.
      //rx_buffer[byte_count - 2] = 0;
      rx_buffer[byte_count] = 0;
      snprintf(cls_message, CLS_LINE_SIZE, "%s", rx_buffer);
      myI2CWriteToLine(I2C2, cls_message, 2);

      // now spit the message back out, formatted to appear visually
      // different from the sent message
      // Note: Cross your fingers and hope that the incoming message wasn't
      // so big that the new format will be truncated in the transmit
      // buffer.
      memset(tx_buffer, 0, MESSAGE_BUFFER_SIZE_BYTES);
      snprintf(tx_buffer, MESSAGE_BUFFER_SIZE_BYTES, "\r\n--You sent: '%s'\r\n", rx_buffer);
      TCPIP_basic_send(SERVER_PORT, tx_buffer, strlen(tx_buffer));
   }
}

// -----------------------------------------------------------------------------
//                    Main
// -----------------------------------------------------------------------------
int main(void)
{
   int generic_ret_val = 0;
   int byte_count = 0;
   UINT8 ip_1 = 0;
   UINT8 ip_2 = 0;
   UINT8 ip_3 = 0;
   UINT8 ip_4 = 0;
   char cls_message[CLS_LINE_SIZE + 1];
   unsigned char rx_buffer[MESSAGE_BUFFER_SIZE_BYTES];
   unsigned char tx_buffer[MESSAGE_BUFFER_SIZE_BYTES];

   unsigned int start_ms = 0;
   unsigned int end_ms = 0;
   unsigned int diff_ms;
   unsigned int diff_count = 0;
   unsigned int sum_diff_ms = 0;
   unsigned int avg_diff_ms = 0;
   unsigned int min_diff_ms = 1000;
   unsigned int max_diff_ms = 0;
   unsigned int ms_count_at_max = 0;

   gMillisecondsInOperation = 0;
   g_TCPIP_service_can_start = FALSE;

   // open the timer that will provide us with simple delay operations
   OpenTimer1(T1_OPEN_CONFIG, T1_TICK_PR);
   ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);

   // MUST be performed before initializing the TCPIP stack, which uses an SPI
   // interrupt to communicate with the wifi pmod
   INTEnableSystemMultiVectoredInt();
   INTEnableInterrupts();

   // ---------------------------- Setpu LEDs ---------------------------------
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);
   PORTClearBits(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);


   setupI2C(I2C2);
   myI2CWriteToLine(I2C2, "I2C init good!", 1);

   function_queue_init();
   myI2CWriteToLine(I2C2, "fnct queue good", 1);

   //TCPIP_and_wifi_stack_init("my_wifi_network_ssid", "my_wifi_network_pass_phrase");
   TCPIP_and_wifi_stack_init("Christ-2.4", "Jesus is GOD!");
   myI2CWriteToLine(I2C2, "TCPIP init good", 1);
   g_TCPIP_service_can_start = TRUE;

   // wait for the wifi to connect
   while(1)
   {
      // I like to blink an LED to show how long this loop is executing
      // Note: This loop will get stuck afteronly a few loops because that is
      // when the TCPIP stack will start connecting to the network, and that
      // takes ~30 seconds, during which you should expect to not see this
      // light blinking.
      add_function_to_queue(blink_onboard_LED_1);

      // make sure to execute the function queue because the TCPIP servicing
      // function, which is what is responsible for connecting to a network, is
      // added to it via an interrupt
      execute_functions_in_queue();

      // break out and continue program once IP address stops being link local
      TCPIP_get_IP_address(&ip_1, &ip_2, &ip_3, &ip_4);
      if (ip_1 != 169 && ip_2 != 254)
      {
         // IP address is not link local, so assume that we are connected to
         // the router specified by the SSID in my TCPIP framework code
         break;
      }
      snprintf(cls_message, CLS_LINE_SIZE, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
      myI2CWriteToLine(I2C2, cls_message, 2);
      
   }

   // open a port on the connected network, and display the result (success or
   // fail) along with the new IP address
   generic_ret_val = TCPIP_open_socket(SERVER_PORT);
   if (generic_ret_val < 0)
   {
      // bad, so print a message and hang here forever (or reset or power loss,
      // whichever comes first)
      snprintf(cls_message, CLS_LINE_SIZE, "port %d fail on", SERVER_PORT);
      while(1);
   }
   else
   {
      snprintf(cls_message, CLS_LINE_SIZE, "port %d open on", SERVER_PORT);
   }
   myI2CWriteToLine(I2C2, cls_message, 1);

   // now the new IP address
   // Note: The new IP address may be shorter, character-wise, than its default
   // address, so clear the CLS line before displaying the new one.
   memset(cls_message, (int)' ', CLS_LINE_SIZE);
   cls_message[CLS_LINE_SIZE - 1] = 0;
   myI2CWriteToLine(I2C2, cls_message, 2);
   snprintf(cls_message, CLS_LINE_SIZE, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
   myI2CWriteToLine(I2C2, cls_message, 2);


   // NOW begin your custom code
   while (1)
   {
      add_function_to_queue(blink_onboard_LED_1);
      //PORTToggleBits(IOPORT_B, BIT_10);

      add_function_to_queue(my_TCPIP_receive_and_reply);

      // execute any functions that may have been added by the timer interrupt
      start_ms = gMillisecondsInOperation;
      execute_functions_in_queue();
      end_ms = gMillisecondsInOperation;
      diff_ms = (end_ms - start_ms);

      sum_diff_ms += diff_ms;
      diff_count += 1;
      avg_diff_ms = sum_diff_ms / diff_count;

      if (diff_ms < min_diff_ms)
      {
         min_diff_ms = diff_ms;
      }
      if (diff_ms > max_diff_ms)
      {
         max_diff_ms = diff_ms;
         ms_count_at_max = gMillisecondsInOperation;
      }

      snprintf(cls_message, CLS_LINE_SIZE, "%max=%d at=%d", max_diff_ms, ms_count_at_max);
      myI2CWriteToLine(I2C2, cls_message, 1);
      snprintf(cls_message, CLS_LINE_SIZE, "curr=%d, avg=%d", diff_ms, avg_diff_ms);
      myI2CWriteToLine(I2C2, cls_message, 2);



//      // I like to blink different LEDs depending on whether there is a
//      // connection or not, and I can get away with it because I am only using
//      // a single socket-port
//      // Note: Blinking LEDs would be a poor way to check the connection status
//      // of a multiple socket-port setup.
//      if (1 == TCPIP_is_there_a_connection_on_port(SERVER_PORT))
//      {
//         PORTClearBits(IOPORT_B, BIT_12);
//         PORTToggleBits(IOPORT_B, BIT_13);
//      }
//      else
//      {
//         PORTClearBits(IOPORT_B, BIT_13);
//         PORTToggleBits(IOPORT_B, BIT_12);
//      }
//
//      byte_count = TCPIP_bytes_in_RX_FIFO(SERVER_PORT);
//      if (byte_count > 0)
//      {
////         myI2CWriteToLine(I2C2, "data Data DATA!", 1);
////         myI2CWriteToLine(I2C2, "bats Bats BATS!", 2);
//
//         // missige for you, sire
//         snprintf(cls_message, CLS_LINE_SIZE, "RX bytes: %d", byte_count);
//         myI2CWriteToLine(I2C2, cls_message, 1);
//
//         // clear out the message buffer, get the missige, then display it on
//         // the CLS pmod, line 2 (because I want "missige sire:" to display on
//         // the top line)
//         // Note: If the message is longer than a single CLS line, that is ok.
//         // It will just start overwriting the last character until the string
//         // is finished.
//         memset(rx_buffer, 0, MESSAGE_BUFFER_SIZE_BYTES);
//         byte_count = TCPIP_basic_receive(SERVER_PORT, rx_buffer, MESSAGE_BUFFER_SIZE_BYTES);
//         if (byte_count > CLS_LINE_SIZE)
//         {
//            // ??do something if it is too long to print on a single CLS line? no? do we care??
//         }
//         else if (byte_count == MESSAGE_BUFFER_SIZE_BYTES)
//         {
//            // do something?
//         }
//
//         // forcefully null terminate the receive buffer to make sure that
//         // formated string (%s) procede without incident
//         // Note: Chop off the last two bytes, which are the return carriage
//         // and new line characters that Tera Term sends when the return key is
//         // pressed.
//         // Note: If not using Tera Term, do not chop off the last two bytes
//         // unless you know that the sending program appends unwanted
//         // characters or other bytes to the end.
//         //rx_buffer[byte_count - 2] = 0;
//         rx_buffer[byte_count] = 0;
//         snprintf(cls_message, CLS_LINE_SIZE, "%s", rx_buffer);
//         myI2CWriteToLine(I2C2, cls_message, 2);
//
//         // now spit the message back out, formatted to appear visually
//         // different from the sent message
//         // Note: Cross your fingers and hope that the incoming message wasn't
//         // so big that the new format will be truncated in the transmit
//         // buffer.
//         memset(tx_buffer, 0, MESSAGE_BUFFER_SIZE_BYTES);
//         snprintf(tx_buffer, MESSAGE_BUFFER_SIZE_BYTES, "\r\n--You sent: '%s'\r\n", rx_buffer);
//         TCPIP_basic_send(SERVER_PORT, tx_buffer, strlen(tx_buffer));
//      }

      // delay for a little bit so that the LED doesn't blink too fast to see
      //delayMS(50);
   }

   return 0;
}


