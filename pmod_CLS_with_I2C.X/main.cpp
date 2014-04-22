extern "C"
{
#include <peripheral/ports.h>	// Enable port pins for input or output
#include <peripheral/system.h>	// Setup the system and perihperal clocks for best performance
#include <peripheral/i2c.h>     // for I2C stuff
#include <peripheral/timer.h>   // for timer stuff
#include <string.h>
//#include <assert.h>
}


/*
 * Jumper setup for rev E CLS pmod
 *
 * MD0: shorted
 * MD1: shorted
 * MD2: open
 *
 * JP1: short for RST (shorting for SS will cause the display to not function
 * under TWI)
 *
 * J4: refers to one SCL pin and one SDA pin
 * J5: ditto with J4
 *
 * J6: refers to one VCC pin and one GND pin
 * J7: ditto with J6
 */


// Oscillator Settings
/*
 * SYSCLK = 80 MHz (8 MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
 * PBCLK = 40 MHz
 * Primary Osc w/PLL (XT+,HS+,EC+PLL)
 * WDT OFF
 * Other options are don't care
 */
#pragma config FNOSC = PRIPLL       // Oscillator selection
#pragma config POSCMOD = EC         // Primary oscillator mode
#pragma config FPLLIDIV = DIV_2     // PLL input divider
#pragma config FPLLMUL = MUL_20     // PLL multiplier
#pragma config FPLLODIV = DIV_1     // PLL output divider
#pragma config FPBDIV = DIV_2       // Peripheral bus clock divider
#pragma config FSOSCEN = OFF        // Secondary oscillator enable

#define SYSTEM_CLOCK            80000000

// Note: This is defined because the #pragma statements are not definitions,
// so we can't use FPBDIV, and therefore we define our own for our period
// calculations
#define PB_DIV              2

// Note: This is defined because Tx_PS_1_SOMEPRSCALAR is a bitshift meant for
// a control register, not the prescalar value itself
#define PS_256              256

// define the timer period constant for the delay timer
#define T1_TOGGLES_PER_SEC  1000
#define T1_TICK_PR          SYSTEM_CLOCK/PB_DIV/PS_256/T1_TOGGLES_PER_SEC
#define T1_OPEN_CONFIG      T1_ON | T1_SOURCE_INT | T1_PS_1_256

#define TWI_ADDR_PMOD_CLS       0x48
// define the frequency (??what kind of frequency? clock frequency? bit transfer frequency? byte transfer frequency??) at which an I2C module will operate
#define I2C_FREQ_1KHZ       100000

// for the CLS; used when formating strings to fit in a line
#define CLS_LINE_SIZE 17


// Globals for setting up pmod CLS
// values in Digilent pmod CLS reference manual, pages 2 - 3
const char enable_display[] =  {27, '[', '3', 'e', '\0'};
const char set_cursor[] =      {27, '[', '1', 'c', '\0'};
const char home_cursor[] =     {27, '[', 'j', '\0'};
const char wrap_line[] =       {27, '[', '0', 'h', '\0'};
const char set_line_two[] =    {27, '[', '1', ';', '0', 'H', '\0'};

unsigned int gMillisecondsInOperation;

extern "C"
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

BOOL moduleIsValid(I2C_MODULE modID)
{
    if (modID != I2C1 && modID != I2C2)
    {
        // invalid module for this board; abort
        return FALSE;
    }

    return TRUE;
}

BOOL setupI2C(I2C_MODULE modID)
{
    // this value stores the return value of I2CSetFrequency(...), and it can
    // be used to compare the actual set frequency against the desired
    // frequency to check for discrepancies
    UINT32 actualClock;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Set the I2C baudrate, then enable the module
    actualClock = I2CSetFrequency(modID, SYSTEM_CLOCK, I2C_FREQ_1KHZ);
    I2CEnable(modID, TRUE);
}

BOOL StartTransferWithoutRestart(I2C_MODULE modID)
{
    // thrashable storage for a return value; used in testing
    unsigned int returnVal;

    // records the status of the I2C module while waiting for it to start
    I2C_STATUS status;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Wait for the bus to be idle, then start the transfer
    while(!I2CBusIsIdle(modID));

    returnVal = I2CStart(modID);
    if(I2C_SUCCESS != returnVal) { return FALSE; }

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(modID);

    } while ( !(status & I2C_START) );

    return TRUE;
}

BOOL StartTransferWithRestart(I2C_MODULE modID)
{
    // thrashable storage for a return value; used in testing
    unsigned int returnVal;

    // records the status of the I2C module while waiting for it to start
    I2C_STATUS status;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Send the Restart) signal (I2C module does not have to be idle)
    returnVal = I2CRepeatStart(modID);
    if(I2C_SUCCESS != returnVal) { return FALSE; }

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(modID);

    } while ( !(status & I2C_START) );

    return TRUE;
}

BOOL StopTransfer(I2C_MODULE modID)
{
    // records the status of the I2C module while waiting for it to stop
    I2C_STATUS status;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Send the Stop signal
    I2CStop(modID);

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(modID);

    } while ( !(status & I2C_STOP) );
}

BOOL TransmitOneByte(I2C_MODULE modID, UINT8 data)
{
    // thrashable storage for a return value; used in testing
    unsigned int returnVal;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // Wait for the transmitter to be ready
    while(!I2CTransmitterIsReady(modID));

    // Transmit the byte
    returnVal = I2CSendByte(modID, data);
    if(I2C_SUCCESS != returnVal) { return FALSE; }

    // Wait for the transmission to finish
    while(!I2CTransmissionHasCompleted(modID));

    // look for the acknowledge bit
    if(!I2CByteWasAcknowledged(modID)) { return FALSE; }

    return TRUE;
}

BOOL TransmitNBytes(I2C_MODULE modID, const char *str, unsigned int bytesToSend)
{
    /*
     * This function performs no initialization of the I2C line or the intended
     * device.  This function is a wrapper for many TransmitOneByte(...) calls.
     */
    unsigned int byteCount;
    unsigned char c;
    BOOL attempt = FALSE;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // check that the number of bytes to send is not bogus
    if (bytesToSend > strlen(str)) { return FALSE; }

    // initialize local variables, then send the string one byte at a time
    byteCount = 0;
    c = *str;
    while(byteCount < bytesToSend)
    {
        // transmit the bytes
        TransmitOneByte(modID, c);
        byteCount++;
        c = *(str + byteCount);
    }

    // transmission successful
    return TRUE;
}

BOOL myI2CWriteToLine(I2C_MODULE modID, const char* string, unsigned int lineNum)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // start the I2C module and signal the CLS
    while(!StartTransferWithoutRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, TWI_ADDR_PMOD_CLS, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }

    // send the cursor selection command, then send the string
    if (2 == lineNum)
    {
        if (!TransmitNBytes(modID, set_line_two, strlen(set_line_two))) { return FALSE; }
    }
    else
    {
        // not line two, so assume line 1
        if (!TransmitNBytes(modID, (char*)home_cursor, strlen(home_cursor))) { return FALSE; }
    }
    if (!TransmitNBytes(modID, string, strlen(string))) { return FALSE; }
    StopTransfer(modID);

    return TRUE;
}

BOOL myI2CInitCLS(I2C_MODULE modID)
{
    I2C_7_BIT_ADDRESS   SlaveAddress;

    // check that we are dealing with a valid I2C module
    if (!moduleIsValid(modID)) { return FALSE; }

    // start the I2C module, signal the CLS, and send setting strings
    while(!StartTransferWithoutRestart(modID));
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, TWI_ADDR_PMOD_CLS, I2C_WRITE);
    if (!TransmitOneByte(modID, SlaveAddress.byte)) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)enable_display, strlen(enable_display))) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)set_cursor, strlen(set_cursor))) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)home_cursor, strlen(home_cursor))) { return FALSE; }
    if (!TransmitNBytes(modID, (char*)wrap_line, strlen(wrap_line))) { return FALSE; }
    StopTransfer(modID);

    // all went well, so return true
    return TRUE;
}




int main(void)
{
    int i;
    char message[20];

    // initialize local variables
    i = 0;
    memset(message, 0, 20);

    // initialize globals
    gMillisecondsInOperation = 0;

    // setup the LEDs
    PORTSetPinsDigitalOut(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);
    PORTClearBits(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);

    // open the timer that will provide us with simple delay operations
    OpenTimer1(T1_OPEN_CONFIG, T1_TICK_PR);
    ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);

    // enable multivector interrupts so the timer1 interrupt vector can lead to
    // the interrupt handler;
    // this also enables interrupts (apparently), thus starting the timer
    INTEnableSystemMultiVectoredInt();

    if (!setupI2C(I2C2))
    {
        PORTSetBits(IOPORT_B, BIT_10);
        while(1);
    }

    if (!myI2CInitCLS(I2C2))
    {
        PORTSetBits(IOPORT_B, BIT_11);
        while(1);
    }
    myI2CWriteToLine(I2C2, "CLS initialized", 1);

    // I2C initialization done; reset onboard LEDs for use in other things
    PORTClearBits(IOPORT_B, BIT_10 | BIT_11 | BIT_12 | BIT_13);

    // loop forever
    i = 0;
    while(1)
    {
        PORTToggleBits(IOPORT_B, BIT_13);
        delayMS(200);

        snprintf(message, CLS_LINE_SIZE, "i = '%d'", i);
        myI2CWriteToLine(I2C2, message, 1);

        i++;
    }
}
