// -----------------------------------------------------------------------------
// Template Notes:
//           -> Microchip TCPIP Tick uses Timer 5, do not use, see "Tick.c"
//           -> prvTCPIP connects WiFi and maintains TCPIP stack
//            this task must be executed frequently, currently it is
//            being serviced every 50ms.
//           -> prvLED toggles an LED every 100ms
//           -> For configuring the TCPIP stack for different
//            functionality see TCPIP.h, TCPIPConfig.h, HardwareProfile.h,
//            WF_Config.h
//           -> INTERRUPTS:
//                The interrupts for Timer 5 and External Interrupt 3
//                are completely setup in Tick.c and WF_Eint.c
//                respectively. Therefore you do not need to setup
//                the vectors, handlers, etc., as they are already
//                taken care of.  If you wish to change them see
//                the ".c" files.  The wrappers for INT3 and Timer 5
//                are in "INT3_ISR.S", "T5_ISR.S" respectively.
//           -> DIGILENT PORT:
//                PmodWiFi should be plugged into Digilent port JB.
//           -> HARDWARE JUMPER:
//                You will need to set the jumper on
//                "JP3" to the "INT3" position.  It is
//                important to note that "INT3" is also a
//                pin on Digilent port "JF", thus you must
//                make sure that you have nothing else
//                connected on the "JF" pin or else you
//                may damage your hardware. The I2C1 bus uses this
//                pin as well, thus make sure you have nothing
//                connected to it (i.e. J2).
//           -> HAVE FUN!!! :D
// -----------------------------------------------------------------------------

#include <peripheral/system.h>
#include <peripheral/timer.h>
#include <peripheral/ports.h>
#include <peripheral/i2c.h>

#include <stdio.h>

#include "my_TCPIP_framework.h"

#include "my_i2c_stuff.h"
#include "my_function_queue.h"



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
   char message[MESSAGE_BUFFER_SIZE_BYTES];
   
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

   // ------------------------ Configure WiFi CS/SS Pin -----------------------
#if defined(WF_CS_TRIS)
   WF_CS_IO = 1;
   WF_CS_TRIS = 0;
#endif

   // Disable JTAG port so we get our I/O pins back, but first
   // wait 50ms so if you want to reprogram the part with
   // JTAG, you'll still have a tiny window before JTAG goes away.
   // The PIC32 Starter Kit debuggers use JTAG and therefore must not
   // disable JTAG.
   //DelayMs(50);
   delayMS(50);
   // -------------------------------------------------------------------------

   setupI2C(I2C2);
   myI2CWriteToLine(I2C2, "I2C init good!", 1);

   function_queue_init();
   myI2CWriteToLine(I2C2, "fnct queue good", 1);

   TCPIP_and_wifi_stack_init();
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
      PORTToggleBits(IOPORT_B, BIT_10);

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
      snprintf(message, CLS_LINE_SIZE, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
      myI2CWriteToLine(I2C2, message, 2);

      // delay for a little bit so that the LED doesn't blink too fast to see
      delayMS(50);
   }

   // open a port on the connected network, and display the result (success or
   // fail) along with the new IP address
   // Note: The new IP address may be shorter, character-wise, than its default
   // address, so clear the CLS line before displaying the new one.
   generic_ret_val = TCPIP_open_socket(SERVER_PORT);
   if (generic_ret_val < 0)
   {
      snprintf(message, CLS_LINE_SIZE, "port %d fail on", SERVER_PORT);
      while(1);
   }
   else
   {
      snprintf(message, CLS_LINE_SIZE, "port %d open on", SERVER_PORT);
   }
   myI2CWriteToLine(I2C2, message, 1);
   
   // now the new IP address
   memset(message, (int)' ', CLS_LINE_SIZE);
   message[CLS_LINE_SIZE - 1] = 0;
   myI2CWriteToLine(I2C2, message, 2);
   snprintf(message, CLS_LINE_SIZE, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
   myI2CWriteToLine(I2C2, message, 2);


   // NOW begin your custom code
   while (1)
   {
      PORTToggleBits(IOPORT_B, BIT_10);

      // execute any functions that may have been added by the timer interrupt
      execute_functions_in_queue();

      // I like to blink different LEDs depending on whether there is a
      // connection or not, and I can get away with it because I am only using
      // a single socket-port
      // Note: Blinking LEDs would be a poor way to check the connection status
      // of a multiple socket-port setup.
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

      byte_count = TCPIP_bytes_in_RX_FIFO(SERVER_PORT);
      if (byte_count > 0)
      {
//         myI2CWriteToLine(I2C2, "data Data DATA!", 1);
//         myI2CWriteToLine(I2C2, "bats Bats BATS!", 2);

         // missige for you, sire
         myI2CWriteToLine(I2C2, "missige sire:", 1);

         // clear out the message buffer, get the missige, then display it on
         // the CLS pmod, line 2 (because I want "missige sire:" to display on
         // the top line)
         // Note: If the message is longer than a single CLS line, that is ok.
         // It will just start overwriting the last character until the string
         // is finished.
         memset(message, 0, MESSAGE_BUFFER_SIZE_BYTES);
         byte_count = TCPIP_basic_receive(SERVER_PORT, message, MESSAGE_BUFFER_SIZE_BYTES);
         if (byte_count > CLS_LINE_SIZE)
         {
            // ??do something if it is too long to print on a single CLS line? no? do we care??
         }
         else if (byte_count == MESSAGE_BUFFER_SIZE_BYTES)
         {
            // do something?
         }

         // chop off the return key, which Tera Term used as a trigger to send
         // the typed text and which included that return key
         message[byte_count - 2] = 0;
         myI2CWriteToLine(I2C2, message, 2);

         // now spit the message back out, unaltered except for the null
         // terminator and a new line character
         // Note: The message buffer is all 0s except for the received
         // characters because of the memset that I did earlier, so I can just
         // add the new line and not worry about having to add another null
         // terminator.
         message[byte_count - 2] = '\r';
         message[byte_count - 1] = '\n';
         TCPIP_basic_send(SERVER_PORT, message, byte_count);
      }

      // delay for a little bit so that the LED doesn't blink too fast to see
      delayMS(50);
   }

   return 0;
}


