/* 
 * File:   main.cpp
 * Author: John
 *
 * Created on April 21, 2014, 10:50 AM
 */

// tell the compiler to include these C-based header files without C++ mangling
extern "C"
{
#include <peripheral/timer.h>
#include <peripheral/int.h>
#include <peripheral/system.h>
#include <peripheral/ports.h>
}

// Configuration Bit settings
// SYSCLK = 80 MHz (8MHz Crystal / FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = SYSCLK / FPBDIV
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF  (??Watch Dog Timer??)
// Other options are don't care
#pragma config FPLLMUL = MUL_20
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLODIV = DIV_1
#pragma config FWDTEN = OFF
#pragma config POSCMOD = HS
#pragma config FNOSC = PRIPLL
#pragma config FPBDIV = DIV_8


// Let compile time pre-processor calculate the timer tick periods (PR)
#define SYS_FREQ            (80000000L)
#define PB_DIV              8
#define PRESCALE            256
#define T1_TOGGLES_PER_SEC  1000
#define T1_TICK_PR          (SYS_FREQ/PB_DIV/PRESCALE/T1_TOGGLES_PER_SEC)

#define LED_PORT           IOPORT_B
#define LED_BIT            BIT_10


static unsigned int gMillisecondsInOperation;

void delayMS(unsigned int milliseconds)
{
   unsigned int millisecondCount = gMillisecondsInOperation;
   while((gMillisecondsInOperation - millisecondCount) < milliseconds);
}

// we are using the XC32 C++ compiler, but this ISR handler registration macro
// only seems to work with the XC32 C compiler, so we have to declare it as
// "extern"
// Note: The first argument is the ISR vector to register the following
// function to.
// Note: The second argument to the macro was designed by Microchip to work
// with the compiler (or maybe it was the preprocessor; I don't know).  It is
// of the form "IPLn(SOFT/SRS/AUTO)".  IPL is "Interrupt Priority Level", "n"
// is level to select and ranges from 1 (highest) to 7 (lowest), and the last
// three specify how the program will perform context switching when the ISR
// vector is called.  When the interrupt happens, the program needs to save
// information about where it was, and it can do this in two ways:
// - SOFT: Store program context on the stack (that is, in software).
// - SRS: Store program context in shadow registers (that is, in hardware),
// which were designed solely to store and resume program context quickly, but
// the downside is that there are only enough to store for one interrupt, so
// choose wisely which interrupts need to happen fast fast fast.
// - AUTO: Let MPLAB X choose between the previous two.
extern "C"
void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
   gMillisecondsInOperation++;

   // clear the interrupt flag
   mT1ClearIntFlag();
}


int main(int argc, char** argv) {
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //STEP 1. Configure cache, wait states and peripheral bus clock
   // Configure the device for maximum performance but do not change the PBDIV
   // Given the options, this function will change the flash wait states, RAM
   // wait state and enable prefetch cache but will not change the PBDIV.
   // The PBDIV value is already set via the pragma FPBDIV option above..
   SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

   // activate the timer that I will use for my "delay milliseconds" function
   OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, T1_TICK_PR);
   ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_1);

   // enable multi-vector interrupts
   INTEnableSystemMultiVectoredInt();

   // Note: See the MX4CK datasheet for LED and button relations to specific
   // ports and pins

   // configure the pin for LEDs LD1 as digital output, and clear it so that
   // the LEDs start off
   PORTSetPinsDigitalOut(LED_PORT, LED_BIT);
   PORTClearBits(LED_PORT, LED_BIT);

   while(1)
   {
      PORTToggleBits(LED_PORT, LED_BIT);

      delayMS(200);
   }

   return 0;
}


