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
BOOL g_TCPIP_send_receive_can_begin;

void TCPIP_send_receive(void);

void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
   // clear the interrupt flag first because the TCPIP stack handling may block
   // for a few dozen seconds when it first connects, and we don't want this
   // timer to stop counting the milliseconds in operation
   mT1ClearIntFlag();

   gMillisecondsInOperation++;

   if (0 == gMillisecondsInOperation % 50)
   {
      // service the TCPIP stack
      add_function_to_queue(TCPIP_keep_stack_alive);
   }
}

void delayMS(unsigned int milliseconds)
{
   unsigned int millisecondCount = gMillisecondsInOperation;
   while ((gMillisecondsInOperation - millisecondCount) < milliseconds);
}

#define SERVER_PORT	22              // Server port

// -----------------------------------------------------------------------------
//                    Main
// -----------------------------------------------------------------------------
int main(void)
{
   int ret_val = 0;
   unsigned int byte_count = 0;
   UINT8 ip_1 = 0;
   UINT8 ip_2 = 0;
   UINT8 ip_3 = 0;
   UINT8 ip_4 = 0;
   char message[20];
   
   gMillisecondsInOperation = 0;
   g_TCPIP_send_receive_can_begin = FALSE;

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



   ret_val = TCPIP_open_socket(SERVER_PORT);
   if (ret_val < 0)
   {
      snprintf(message, CLS_LINE_SIZE, "open port %d fail", SERVER_PORT);
      while(1);
   }
   else
   {
      snprintf(message, CLS_LINE_SIZE, "port %d opened", SERVER_PORT);
   }
   myI2CWriteToLine(I2C2, message, 1);

   while (1)
   {
      PORTToggleBits(IOPORT_B, BIT_10);

      execute_next_function_in_queue();

      byte_count = TCPIP_bytes_in_RX_FIFO(SERVER_PORT);
      if (byte_count > 0)
      {
         myI2CWriteToLine(I2C2, "data Data DATA!", 1);
         //snprintf(message, CLS_LINE_SIZE, "%s", g_wifi_message_structure.rxBuf);
         //myI2CWriteToLine(I2C2, message, 1);
      }
      else
      {
         //myI2CWriteToLine(I2C2, "no data; sad", 1);
      }

      TCPIP_get_IP_address(&ip_1, &ip_2, &ip_3, &ip_4);
      if (ip_1 != 169 && ip_2 != 254)
      {
         // we have an IP address that is other than local linking, so assume
         // that we are connected to a router
         g_TCPIP_send_receive_can_begin = TRUE;
      }
      snprintf(message, CLS_LINE_SIZE, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
      myI2CWriteToLine(I2C2, message, 2);


      delayMS(50);      
   }

   return 0;
}


