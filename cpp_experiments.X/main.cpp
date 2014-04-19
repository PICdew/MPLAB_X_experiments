// much of this code, especially the #pragmas and #defines and system and timer
// configurations, were taken from the following file:
// C:\Program Files (x86)\Microchip\xc32\v1.20\examples\plib_examples\timer\timer1_int\source\timer1_int.c

// ??can this be made more specific??
#include <plib.h>

// Configuration Bit settings
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 40 MHz
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF  (??Watch Dog Timer??)
// Other options are don't care
//
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_8


// Let compile time pre-processor calculate the timer tick periods (PR)
#define SYS_FREQ         (80000000L)
#define PB_DIV           8
#define PRESCALE         256
#define T1_TOGGLES_PER_SEC  1000
#define T2_TOGGLES_PER_SEC  10
#define T3_TOGGLES_PER_SEC  30
#define T1_TICK_PR        (SYS_FREQ/PB_DIV/PRESCALE/T1_TOGGLES_PER_SEC)
#define T2_TICK_PR        (SYS_FREQ/PB_DIV/PRESCALE/T2_TOGGLES_PER_SEC)
#define T3_TICK_PR        (SYS_FREQ/PB_DIV/PRESCALE/T2_TOGGLES_PER_SEC)

// globals (??consider revising so there are no globals??)
int button_1_timer_active;
int button_2_timer_active;

void handleButton1(void);
void handleButton2(void);
void delayMS(unsigned int milliseconds);

int main(void)
{
   unsigned int port_state = 0;
   unsigned int button_states = 0;

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //STEP 1. Configure cache, wait states and peripheral bus clock
   // Configure the device for maximum performance but do not change the PBDIV
   // Given the options, this function will change the flash wait states, RAM
   // wait state and enable prefetch cache but will not change the PBDIV.
   // The PBDIV value is already set via the pragma FPBDIV option above..
   SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

   button_1_timer_active = 0;
   button_2_timer_active = 0;

   // activate the timer that I will use for my "delay milliseconds" function
   OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, T1_TICK_PR);
   ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_1);

   // enable multi-vector interrupts
   INTEnableSystemMultiVectoredInt();

   // Note: See the MX4CK datasheet for LED and button relations to specific
   // ports and pins

   // configure the pins for the two buttons on the MX4CK as digital inputs
   PORTSetPinsDigitalIn(IOPORT_A, BIT_6 | BIT_7);

   // configure the pins for LEDs LD1 and LD4 as digital outputs, and clear the
   // bits so that the LEDs start off
   PORTSetPinsDigitalOut(IOPORT_B, BIT_10 | BIT_13);
   PORTClearBits(IOPORT_B, BIT_10 | BIT_13);

   while(1)
   {
      // read the states of the pins feeding in from the buttons
      port_state = PORTRead(IOPORT_A);

      // mask out everything but pins 6 and 7 on port A, as per the Cerebot
      // MX4cK reference manual, pg 10
      button_states = port_state & 0x000000C0;

      if (0x000000C0 == button_states)
      {
         // both buttons pressed;
         // handle them both
         handleButton1();
         handleButton2();
      }
      else if (0x00000040 == button_states)
      {
         // only button 1 pressed
         handleButton1();
      }
      else if (0x00000080 == button_states)
      {
         // only button 2 pressed
         handleButton2();
      }

      // delay a short time so that we don't read a single button press
      // multiple times, assuming of course that the user only intended to
      // quickly push the button once
      delayMS(200);
   }
}

unsigned int gMillisecondsInOperation;

void delayMS(unsigned int milliseconds)
{
   unsigned int millisecondCount = gMillisecondsInOperation;
   while((gMillisecondsInOperation - millisecondCount) < milliseconds);
}

void handleButton1(void)
{
   // pressing this button the first time turns on a timer that toggles an LED,
   // and pressing it again turns it off
   if (button_1_timer_active)
   {
      CloseTimer2();
      PORTClearBits(IOPORT_B, BIT_10);
      button_1_timer_active = 0;
   }
   else
   {
      OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256, T2_TICK_PR);
      ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
      button_1_timer_active = 1;
   }
}

void handleButton2(void)
{
   // same as "handle button 1", but with a different timer and different LED
   if (button_2_timer_active)
   {
      CloseTimer3();
      PORTClearBits(IOPORT_B, BIT_13);
      button_2_timer_active = 0;
   }
   else
   {
      OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, T3_TICK_PR);
      ConfigIntTimer3(T3_INT_ON | T3_INT_PRIOR_2);
      button_2_timer_active = 1;
   }
}

// Note: SPECIFYING JUST "IPL7" IS INSUFFICIENT!  AUTO, SOFT, OR HARD MUST BE
// SPECIFIED!  I AM TYPING THIS IN ALL CAPS BECAUSE I WANT YOU TO PAY ATTENTION
// AND REMEMBER THIS LITTLE THING THAT COST YOU SO MUCH DEBUGGING TIME!
extern "C"
void __ISR(_TIMER_1_VECTOR, IPL7AUTO) Timer1Handler(void)
{
   gMillisecondsInOperation++;

   // clear the interrupt flag
   mT1ClearIntFlag();
}

extern "C"
void __ISR(_TIMER_2_VECTOR, IPL7SOFT) Timer2Handler(void)
{
   // clear the interrupt flag
   mT2ClearIntFlag();  //??difference with T1ClearIntFlag()? no 'm' prefix??

   // .. things to do
   // .. in this case, toggle the LED
   mPORTBToggleBits(BIT_10);
}

extern "C"
void __ISR(_TIMER_3_VECTOR, IPL7SOFT) Timer3Handler(void)
{
   // clear the interrupt flag
   mT3ClearIntFlag();  //??vs mT1...??

   // .. things to do
   // .. in this case, toggle the LED
   mPORTBToggleBits(BIT_13);
}