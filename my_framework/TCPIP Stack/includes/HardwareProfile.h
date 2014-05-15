 /*********************************************************************
 *
 *	Hardware specific definitions for:
 *    - Digilent Cerebot MX7ck
 *
 *********************************************************************
 * FileName:        HardwareProfile.h
 * Dependencies:    Compiler.h
 * Processor:       PIC32
 * Compiler:        Microchip C32 v1.11 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2010 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder		09/16/2010	Regenerated for specific boards
 ********************************************************************/
#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#include "Compiler.h"

// Define a macro describing this hardware set up (used in other files)
#define MX4ck

// Set configuration fuses (but only in MainDemo.c where THIS_IS_STACK_APPLICATION is defined)
// SYSCLK = 80 MHz (8 MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 80 MHz
//#if defined(THIS_IS_STACK_APPLICATION)
//	#pragma config FPLLODIV = DIV_1, FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FWDTEN = OFF, FPBDIV = DIV_2, POSCMOD = XT, FNOSC = PRIPLL, CP = OFF
//#endif


// Clock frequency values
// These directly influence timed events using the Tick module.  They also are used for UART and SPI baud rate generation.
#define GetSystemClock()		(80000000ul)			// Hz
#define GetInstructionClock()	(GetSystemClock()/1)	// Normally GetSystemClock()/4 for PIC18, GetSystemClock()/2 for PIC24/dsPIC, and GetSystemClock()/1 for PIC32.  Might need changing if using Doze modes.
#define GetPeripheralClock()	(GetSystemClock()/2)	// Normally GetSystemClock()/4 for PIC18, GetSystemClock()/2 for PIC24/dsPIC, and GetSystemClock()/1 for PIC32.  Divisor may be different if using a PIC32 since it's configurable.


// +++++++++++++++++++ Hardware I/O pin mappings +++++++++++++++++++++++++++++++

// ======================== For Cerebot MX4ck ==================================
// LEDs
#define LED0_TRIS			(TRISBbits.TRISB10)	// LD1
#define LED0_IO				(LATBbits.LATB10)
#define LED1_TRIS			(TRISBbits.TRISB11)	// LD2
#define LED1_IO				(LATBbits.LATG11)
#define LED2_TRIS			(TRISBbits.TRISB12)	// LD3
#define LED2_IO				(LATBbits.LATB12)
#define LED3_TRIS			(TRISBbits.TRISB13)	// LD4
#define LED3_IO				(LATBbits.LATB13)
#define LED4_TRIS			(LED3_TRIS)			// No such LED
#define LED4_IO				(LATBbits.LATB10)
#define LED5_TRIS			(LED3_TRIS)			// No such LED
#define LED5_IO				(LATBbits.LATB11)
#define LED6_TRIS			(LED3_TRIS)			// No such LED
#define LED6_IO				(LATBbits.LATB12)
#define LED7_TRIS			(LED3_TRIS)			// No such LED
#define LED7_IO				(LATBbits.LATB13)
#define LED_GET()			((unsigned char)((LATB & 0xF000)>>10))
#define LED_PUT(a)			do{LATB = (LATB & 0x3CFF) | (((a)&0x0F)<<10);}while(0)

// Momentary push buttons
#define BUTTON0_TRIS		(TRISAbits.TRISA6)	// BTN1
#define BUTTON0_IO			(PORTAbits.RA6)
#define BUTTON1_TRIS		(TRISAbits.TRISA7)	// BTN2
#define BUTTON1_IO			(PORTAbits.RA7)
#define BUTTON2_TRIS		(TRISAbits.TRISA00)	// BTN3
#define BUTTON2_IO			(1)
#define BUTTON3_TRIS		(TRISAbits.TRISA00)	// No BUTTON4 on this board
#define BUTTON3_IO			(1)

// UART configuration (not too important since we don't have a UART
// connector attached normally, but needed to compile if the STACK_USE_UART
// or STACK_USE_UART2TCP_BRIDGE features are enabled.
// LSH this feature doesn't work properly on Cerebot 32MX7
    #define UARTTX_TRIS			(TRISFbits.TRISF8) //changed the uart to uart 2 so it doesn't overwrite the spi3 ports
    #define UARTRX_TRIS			(TRISFbits.TRISF2)



// Specify which SPI to use for the ENC28J60 or ENC624J600.  SPI1 is
// the topmost slot with pin 1 on it.  SPI2 is the middle slot
// starting on pin 33.
//#define ENC_IN_SPI1
//#define ENC_IN_SPI2

// Note that SPI1 cannot be used when using the PIC32 USB Starter
// Board or PIC32 USB Starter Kit II due to the USB peripheral pins
// mapping on top of the ordinary SPI1 pinout.
#if defined(ENC_IN_SPI1) && (defined(__32MX460F512L__) || defined(__32MX795F512L__))
	#undef ENC_IN_SPI1
	#define ENC_IN_SPI2
#endif




//------------------------------------------------------------------------------
//                              PmodWiFi IO Pins
//------------------------------------------------------------------------------
    //  JF, SPI4 or SPI3A, INT2
    #define MRF24WB0M_IN_SPI2
    #define MRF24WB0M_IN_SPI_INT3

    // set the Pmod connector JB bits
	#define WF_CS_TRIS			(TRISGbits.TRISG9)      // SS/CS
	#define WF_CS_IO			(LATGbits.LATG9)        // MOSI
	#define WF_SDI_TRIS			(TRISGbits.TRISG7)      // MISO
	#define WF_SCK_TRIS			(TRISGbits.TRISG6)      // SCLK
	#define WF_SDO_TRIS			(TRISGbits.TRISG8)
	#define WF_RESET_TRIS                   (TRISDbits.TRISD5)
	#define WF_RESET_IO			(LATDbits.LATD5)
	#define WF_INT_TRIS			(TRISAbits.TRISA14)  // INT3, SET JP3 TO INT3
	#define WF_INT_IO			(PORTAbits.RA14)
	#define WF_HIBERNATE_TRIS               (TRISBbits.TRISB14)
	#define WF_HIBERNATE_IO                 (PORTBbits.RB14)
	
	// Set the SPI interrupt to use; Only defined for INT 1,2,3... other aren't setup to be used.
	#if defined(MRF24WB0M_IN_SPI_INT1)
		#define WF_INT_EDGE			(INTCONbits.INT1EP)
		#define WF_INT_IE			(IEC0bits.INT1IE)
		#define WF_INT_IF			(IFS0bits.INT1IF)
		#define WF_INT_BIT			0x00000080		    // INT 1
		#define WF_INT_IPCSET		IPC1SET
		#define WF_INT_IPCCLR		IPC1CLR
	#elif defined(MRF24WB0M_IN_SPI_INT2)
		#define WF_INT_EDGE			(INTCONbits.INT2EP)
		#define WF_INT_IE			(IEC0bits.INT2IE)
		#define WF_INT_IF			(IFS0bits.INT2IF)
		#define WF_INT_BIT			0x00000800		    // INT 2
		#define WF_INT_IPCSET		IPC2SET
		#define WF_INT_IPCCLR		IPC2CLR
	#elif defined(MRF24WB0M_IN_SPI_INT3)
		#define WF_INT_EDGE			(INTCONbits.INT3EP)
		#define WF_INT_IE			(IEC0bits.INT3IE)
		#define WF_INT_IF			(IFS0bits.INT3IF)
		#define WF_INT_BIT			0x00008000	        // INT 3	
		#define WF_INT_IPCSET                   IPC3SET
		#define WF_INT_IPCCLR                   IPC3CLR
	#else
		#error unsupported MRF24WB0M SPI interrupt
	#endif

		// common SPI interrupt stuff; careful only good for INT 1,2,3
		#define WF_INT_IE_CLEAR		IEC0CLR
		#define WF_INT_IF_CLEAR		IFS0CLR
		#define WF_INT_IE_SET		IEC0SET
		#define WF_INT_IF_SET		IFS0SET
		#define WF_INT_IPC_MASK		0xFF000000
		#define WF_INT_IPC_VALUE        0x0C000000

	// setup for SPI 1,2,3,4  Only the MX7cK and Max32 support SPI 3 & 4.
	#if defined(MRF24WB0M_IN_SPI1)
		#define WF_SSPBUF			(SPI1BUF)
		#define WF_SPISTAT			(SPI1STAT)
		#define WF_SPISTATbits		(SPI1STATbits)
		#define WF_SPICON1			(SPI1CON)
		#define WF_SPICON1bits		(SPI1CONbits)
		#define WF_SPI_BRG			(SPI1BRG)
		#define WF_SPI_INT_BITS		0x03800000  // SPI1 
		#define WF_SPI_IE_CLEAR		IEC0CLR
		#define WF_SPI_IF_CLEAR		IFS0CLR
	#elif defined(MRF24WB0M_IN_SPI2)
		#define WF_SSPBUF			(SPI2BUF)
		#define WF_SPISTAT			(SPI2STAT)
		#define WF_SPISTATbits                  (SPI2STATbits)
		#define WF_SPICON1			(SPI2CON)
		#define WF_SPICON1bits                  (SPI2CONbits)
		#define WF_SPI_BRG			(SPI2BRG)
		#define WF_SPI_INT_BITS                 0x000000E0  // SPI2 and I think SPI2A
		#define WF_SPI_IE_CLEAR                 IEC1CLR
		#define WF_SPI_IF_CLEAR                 IFS1CLR
	#elif defined(MRF24WB0M_IN_SPI3)
		#define WF_SSPBUF			(SPI3BUF)
		#define WF_SPISTAT			(SPI3STAT)
		#define WF_SPISTATbits		(SPI3STATbits)
		#define WF_SPICON1			(SPI3CON)
		#define WF_SPICON1bits		(SPI3CONbits)
		#define WF_SPI_BRG			(SPI3BRG)
		#define WF_SPI_INT_BITS		0x1C000000  // SPI3 or SPI1A  
		#define WF_SPI_IE_CLEAR		IEC0CLR
		#define WF_SPI_IF_CLEAR		IFS0CLR
	#elif defined(MRF24WB0M_IN_SPI4)
		#define WF_SSPBUF			(SPI4BUF)
		#define WF_SPISTAT			(SPI4STAT)
		#define WF_SPISTATbits		(SPI4STATbits)
		#define WF_SPICON1			(SPI4CON)
		#define WF_SPICON1bits		(SPI4CONbits)
		#define WF_SPI_BRG			(SPI4BRG)
		#define WF_SPI_INT_BITS		0x00000700  // SPI4 or SPI3A 
		#define WF_SPI_IE_CLEAR		IEC1CLR
		#define WF_SPI_IF_CLEAR		IFS1CLR
	#else
		#error unsupported MRF24WB0M SPI port
	#endif

	#define WF_MAX_SPI_FREQ		(10000000ul)	// Hz


#endif // #ifndef HARDWARE_PROFILE_H
