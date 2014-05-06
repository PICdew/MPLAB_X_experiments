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

   if (0 == gMillisecondsInOperation % 200)
   {
      if (g_TCPIP_send_receive_can_begin)
      {
         PORTClearBits(IOPORT_B, BIT_11);
         add_function_to_queue(TCPIP_send_receive);
      }
      else
      {
         PORTToggleBits(IOPORT_B, BIT_11);
      }
   }
}

void delayMS(unsigned int milliseconds)
{
   unsigned int millisecondCount = gMillisecondsInOperation;
   while ((gMillisecondsInOperation - millisecondCount) < milliseconds);
}

#define SERVER_PORT	22              // Server port
#define WIFI_MSG_BUF_SIZE 150
typedef struct MSG_WIFI         // Structure for prvRxTx messages
{
   BYTE rxBuf[WIFI_MSG_BUF_SIZE];
   BYTE txBuf[WIFI_MSG_BUF_SIZE];
} MSG_WIFI;

MSG_WIFI g_wifi_message_structure;

void TCPIP_send_receive(void)
{
   char message[20];

   static TCP_SOCKET	MySocket;
   static enum _TCPServerState
   {
      SM_HOME = 0,
      SM_LISTENING,
      SM_CLOSING,
   } TCPServerState = SM_HOME;

   static WORD prev_rx_byte_count = 0;
   WORD curr_rx_byte_count = 0;
   WORD bytes_read = 0;

   WORD space_in_tx_buffer = 0;

   switch (TCPServerState)
   {
      // ---------------------------- HOME -------------------------------
   case SM_HOME:
      // Allocate a socket for this server to listen and accept connections on
      MySocket = TCPOpen(0, TCP_OPEN_SERVER, SERVER_PORT, TCP_PURPOSE_GENERIC_TCP_SERVER);
      if (MySocket == INVALID_SOCKET)
      {
         myI2CWriteToLine(I2C2, "bad socket", 1);
      }
      else
      {
         myI2CWriteToLine(I2C2, "socket is good", 1);
         TCPServerState = SM_LISTENING;
      }

      break;

   case SM_LISTENING:
      // See if anyone is connected to us
      if (!TCPIsConnected(MySocket))
      {
         PORTClearBits(IOPORT_B, BIT_12);
         PORTToggleBits(IOPORT_B, BIT_13);
      }
      else
      {
         PORTClearBits(IOPORT_B, BIT_13);
         PORTToggleBits(IOPORT_B, BIT_12);
      }

      // -------------------------------------------------------------
      //                      Read Rx Data
      // -------------------------------------------------------------
      curr_rx_byte_count = TCPIsGetReady(MySocket);
      if (0 != curr_rx_byte_count)
      {
         // there is data available, but is the data still incoming?
         if (curr_rx_byte_count == prev_rx_byte_count)
         {
            // data has stopped coming, so clear the receive buffer and read
            // the data
            memset(g_wifi_message_structure.rxBuf, 0, WIFI_MSG_BUF_SIZE);
            bytes_read = TCPGetArray(MySocket, g_wifi_message_structure.rxBuf, WIFI_MSG_BUF_SIZE);
            //g_wifi_message_structure.rxBuf[bytes_read - 1] = 0;
         }
      }
      prev_rx_byte_count = curr_rx_byte_count;

      // -------------------------------------------------------------
      //                      Send Tx Data
      // -------------------------------------------------------------
      space_in_tx_buffer = TCPIsPutReady(MySocket);
      if (space_in_tx_buffer >= WIFI_MSG_BUF_SIZE)
      {
         // space is available, so send out a message, then clear out the
         // transmit buffer
         strncpy(g_wifi_message_structure.txBuf, "hi there!", WIFI_MSG_BUF_SIZE);
         TCPPutArray(MySocket, g_wifi_message_structure.txBuf, WIFI_MSG_BUF_SIZE);
         memset(g_wifi_message_structure.txBuf, 0, WIFI_MSG_BUF_SIZE);
      }

      break;

      // ----------------------- CLOSING -----------------------------
   case SM_CLOSING:
      // Close the socket connection.
      TCPClose(MySocket);

      TCPServerState = SM_HOME;
      break;
   }
}

// -----------------------------------------------------------------------------
//                    Main
// -----------------------------------------------------------------------------
int main(void)
{
   UINT32 t = 0;
   int i = 0;
   UINT8 ip_1 = 0;
   UINT8 ip_2 = 0;
   UINT8 ip_3 = 0;
   UINT8 ip_4 = 0;
   char message[20];
   
   gMillisecondsInOperation = 0;
   g_TCPIP_send_receive_can_begin = FALSE;

   function_queue_init();

   // open the timer that will provide us with simple delay operations
   OpenTimer1(T1_OPEN_CONFIG, T1_TICK_PR);
   ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);

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
   myI2CWriteToLine(I2C2, "yay I2C init good!", 1);

   TickInit();
   myI2CWriteToLine(I2C2, "tick init done", 1);

   // initialize the basic application configuration
   InitAppConfig();
   myI2CWriteToLine(I2C2, "app init done", 1);

   // Initialize the core stack layers
   StackInit();
   myI2CWriteToLine(I2C2, "stack init done", 1);

#if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
   g_WpsPassphrase.valid = FALSE;
#endif   /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */
   WF_Connect();
   myI2CWriteToLine(I2C2, "WF connect done", 1);

   while (1)
   {
      // perform normal stack tasks including checking for incoming
      // packets and calling appropriate handlers
      StackTask();

      #if defined(WF_CS_TRIS)
         #if !defined(MRF24WG)
            if (gRFModuleVer1209orLater)
         #endif
         WiFiTask();
      #endif

      // This tasks invokes each of the core stack application tasks
      StackApplications();


      // -------------- Custom Code Here -----------------------------

      PORTToggleBits(IOPORT_B, BIT_10);

      execute_next_function_in_queue();

      int i = strlen(g_wifi_message_structure.rxBuf);
      if (i > 0)
      {
         myI2CWriteToLine(I2C2, "data Data DATA!", 1);
         //snprintf(message, CLS_LINE_SIZE, "%s", g_wifi_message_structure.rxBuf);
         //myI2CWriteToLine(I2C2, message, 1);
      }
      else
      {
         //myI2CWriteToLine(I2C2, "no data; sad", 1);
      }

      // extract the sections of the IP address and print them
      // Note: Network byte order, including IP addresses, are "big endian",
      // which means that their most significant bit is in the least
      // significant bit position.
      // Ex: IP address is 169.254.1.1.  Then "169" is the first byte, "254" is
      // 8 bits after that, etc.
      ip_4 = AppConfig.MyIPAddr.byte.MB;     // highest byte (??MB??), fourth number
      ip_3 = AppConfig.MyIPAddr.byte.UB;     // middle high byte (??UB??), third number
      ip_2 = AppConfig.MyIPAddr.byte.HB;     // middle low byte (??HB??), second number
      ip_1 = AppConfig.MyIPAddr.byte.LB;     // lowest byte, first number
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


