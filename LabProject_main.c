// Filename:            Lab2Idle_main.c
//
// Description:         This file has a main, timer, and idle function for SYS/BIOS application.
//
// Target:              TMS320F28379D
//
// Author:              DR
//
// Date:                Oct. 12, 2021
//
// Modified By:         Matthew Peeters, A01014378
// Modification Date:   2021-10-13
// Added myIdleFxn2 to print to print to the SysMin OutputBuffer every second.

//defines:
#define xdc__strict //suppress typedef warnings
#define buffer_length 10000
#define num_buffers 2

//includes:
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>

#include <Headers/F2837xD_device.h>

//function prototypes:
extern void DeviceInit(void);

//declare global variables:
volatile Bool isrFlag = FALSE; //flag used by idle function
volatile Bool isrFlag2 = FALSE; // MP - flag used by idle2 function
volatile UInt i = 0; // MP - Seconds counter
volatile Int tickCount = 0; //counter incremented by timer interrupt

/* ---- Declare Buffer ---- */
// Having a buffer (or struct) longer than ~10,000 elements
// throws an error due to how the RAM is addressed
volatile int16 sample_buffer[buffer_length];

volatile Uint16 buffer_i = 0; // Current index of buffer

volatile Bool newSampleFlag = FALSE;

/* ======== main ======== */
Int main()
{ 
    System_printf("Enter main()\n"); //use ROV->SysMin to view the characters in the circular buffer

    //initialization:
    DeviceInit(); //initialize processor

    //jump to RTOS (does not return):
    BIOS_start();
    return(0);
}

/* ======== myTickFxn ======== */
//Timer tick function that increments a counter and sets the isrFlag
//Entered 100 times per second if PLL and Timer set up correctly
Void myTickFxn(UArg arg)
{
    tickCount++; //increment the tick counter
    if(tickCount % 5 == 0) {
        isrFlag = TRUE; //tell idle thread to do something 20 times per second
    }
    if(tickCount % 100 == 0){ // MP
        isrFlag2 = TRUE; // MP
    } // MP
}

/* ======== myIdleFxn ======== */
//Idle function that is called repeatedly from RTOS
Void myIdleFxn(Void)
{
   if(isrFlag == TRUE) {
       isrFlag = FALSE;
       //toggle blue LED:
       GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
   }
}

Void myIdleFxn2(Void) // MP
{ // MP
    if(isrFlag2 == TRUE){ // MP
        isrFlag2 = FALSE; // MP
        System_printf("Time (sec) = %u \n", i); // MP
        i++; // MP
    } // MP
} // MP

/* ======== effect_bitCrush ======== */
// Reduces the resolution of x to the specified
// number of bits (m).
//
// Parameters:
// *x - The address of the sample to reduce the resolution.
// m - The desired number of bit resolution.
// N - The total number of bits in the original sample.
Void effect_bitCrush(UInt *x, UInt m, UInt N){
    // Calculate number of bits to shift by
    UInt shift = N - m;

    // Shift right to reduce bit resolution,
    // shift back to original number of bits
    *x = (*x >> shift) << shift;
}

/* ======== effect_echo ======== */
//Void effect_echo(UInt *x_arr){

//}

Void audioIn_hwi(Void)
{
    int16 input_sample;

    input_sample = AdcaResultRegs.ADCRESULT0; //get reading from ADC

    // Store sample into the next buffer slot
    sample_buffer[buffer_i] = input_sample;

    // Circular buffer indexing
    if(buffer_i >= buffer_length - 1) buffer_i = 0;
    else buffer_i++;


    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear interrupt flag

    // Set flag indicating new sample has been captured
    newSampleFlag = TRUE;
}

