// -----------------------------------------------------------------------------
// Template Notes:
//              -> Microchip TCPIP Tick uses Timer 5, do not use, see "Tick.c"
//              -> prvTCPIP connects WiFi and maintains TCPIP stack
//                this task must be executed frequently, currently it is
//                being serviced every 50ms.
//              -> prvLED toggles an LED every 100ms
//              -> For configuring the TCPIP stack for different
//                functionality see TCPIP.h, TCPIPConfig.h, HardwareProfile.h,
//                WF_Config.h
//              -> INTERRUPTS:
//                     The interrupts for Timer 5 and External Interrupt 3
//                     are completely setup in Tick.c and WF_Eint.c
//                     respectively. Therefore you do not need to setup
//                     the vectors, handlers, etc., as they are already
//                     taken care of.  If you wish to change them see
//                     the ".c" files.  The wrappers for INT3 and Timer 5
//                     are in "INT3_ISR.S", "T5_ISR.S" respectively.
//              -> DIGILENT PORT:
//                     PmodWiFi should be plugged into Digilent port JB.
//              -> HARDWARE JUMPER:
//                     You will need to set the jumper on
//                     "JP3" to the "INT3" position.  It is
//                     important to note that "INT3" is also a
//                     pin on Digilent port "JF", thus you must
//                     make sure that you have nothing else
//                     connected on the "JF" pin or else you
//                     may damage your hardware. The I2C1 bus uses this
//                     pin as well, thus make sure you have nothing
//                     connected to it (i.e. J2).
//              -> HAVE FUN!!! :D
// -----------------------------------------------------------------------------

#include <peripheral/timer.h>
#include <peripheral/i2c.h>
#include "i2c_stuff.h"
#include <stdio.h>

// --------------------- TCPIP WiFi Stuff ---------------------------------------
#define ROUTER_SSID                     "Christ-2.4"
#define PASSPHRASE                      "Jesus is GOD!"        // WPA2 encryption
#include "MCHP_TCPIP.h"
// -----------------------------------------------------------------------------

// ------------------ Configuration Oscillators --------------------------------
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK  = 40 MHz
// -----------------------------------------------------------------------------
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_2
//#pragma config CP = OFF, BWP = OFF, PWP = OFF
//#pragma config UPLLEN = OFF


#define SYSTEM_CLOCK            80000000
#define PB_DIV              2
#define PS_256              256
#define T1_TOGGLES_PER_SEC  1000
#define T1_TICK_PR          SYSTEM_CLOCK/PB_DIV/PS_256/T1_TOGGLES_PER_SEC
#define T1_OPEN_CONFIG      T1_ON | T1_SOURCE_INT | T1_PS_1_256

unsigned int gMillisecondsInOperation;

void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
    gMillisecondsInOperation++;

    // clear the interrupt flag
    mT1ClearIntFlag();
}

void delayMS(unsigned int milliseconds)
{
    unsigned int millisecondCount = gMillisecondsInOperation;
    while((gMillisecondsInOperation - millisecondCount) < milliseconds);
}




// -----------------------------------------------------------------------------
//                          Main
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

   // open the timer that will provide us with simple delay operations
   OpenTimer1(T1_OPEN_CONFIG, T1_TICK_PR);
   ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);

   // ---------------------------- Setpu LEDs ---------------------------------
      PORTSetPinsDigitalOut (IOPORT_B, BIT_10 |  BIT_11 |  BIT_12 |  BIT_13);
      PORTClearBits (IOPORT_B,  BIT_10 |  BIT_11 |  BIT_12 |  BIT_13);

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
      DelayMs(50);
   // -------------------------------------------------------------------------

   setupI2C(I2C2);
   myI2CWriteToLine(I2C2, "yay!", 1);

   INTEnableSystemMultiVectoredInt();
   INTEnableInterrupts();

   TickInit();

   // initialize the basic application configuration
   InitAppConfig();

   // Initialize the core stack layers
   StackInit();

      #if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
         g_WpsPassphrase.valid = FALSE;
      #endif   /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */
   WF_Connect();

   while (1)
   {
      i += 1;
      PORTToggleBits(IOPORT_B, BIT_10);
      snprintf(message, CLS_LINE_SIZE, "%i", i);
      myI2CWriteToLine(I2C2, message, 1);

      // extract the sections of the IP address and print them
      // Note: Network byte order, including IP addresses, are "big endian",
      // which means that their most significant bit is in the least
      // significant bit position.
      // Ex: IP address is 169.254.1.1.  Then "169" is the first byte, "254" is
      // 8 bits after that, etc.
//      ip_4 = (AppConfig.MyIPAddr.Val & 0xFF000000) >> 24;
//      ip_3 = (AppConfig.MyIPAddr.Val & 0x00FF0000) >> 16;
//      ip_2 = (AppConfig.MyIPAddr.Val & 0x0000FF00) >> 8;
//      ip_1 = (AppConfig.MyIPAddr.Val & 0x000000FF) >> 0;
      ip_4 = AppConfig.MyIPAddr.byte.MB;
      ip_3 = AppConfig.MyIPAddr.byte.UB;
      ip_2 = AppConfig.MyIPAddr.byte.HB;
      ip_1 = AppConfig.MyIPAddr.byte.LB;
      snprintf(message, CLS_LINE_SIZE, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
      myI2CWriteToLine(I2C2, message, 2);


      DelayMs(50);
      //delayMS(50);

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
   }

   return 0;
}


