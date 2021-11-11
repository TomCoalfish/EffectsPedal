// Filename:            EffectsPedal_main.c
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
//                      Kieran Bako, A01028276
// Modification Date:   2021-11-06
//defines:
#define xdc__strict //suppress typedef warnings
#define buffer_length 9000
#define fs 48000 // Sampling Frequency - 48kHz
#define N_bits 12 // Bit resolution of input samples

//includes:
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <Headers/F2837xD_device.h>
#include <math.h>
#include <bandpass_coeffs.h>

//Swi Handle defined in .cfg file:
extern const Swi_Handle audioOut_swi_handle;

//Tsk Handle defined in .cfg file:
extern const Task_Handle task0;

//Semaphores defined in .cfg file:
extern const Semaphore_Handle sem0;

//Declare global variables:
volatile Bool isrFlag = FALSE; //flag used by idle function
volatile UInt tickCount = 0; //counter incremented by timer interrupt

volatile int effect_num = 0;

/* ---- Declare Buffer ---- */
// Having a buffer (or struct) longer than ~10,000 elements
// throws an error due to how the RAM is addressed
volatile Uint16 sample_buffer[buffer_length];
volatile Uint16 buffer_i = 0; // Current index of buffer

//function prototypes:
extern void DeviceInit(void);
void (*audio_effect)(UInt *, volatile UInt16 *, UInt16); // Declare pointer to function
void effect_bitCrush(UInt16 *y, volatile UInt16 *x, UInt16 m);
void effect_echo(UInt16 *y, volatile UInt16 *x, UInt16 m);
void effect_chorus(UInt16 *y, volatile UInt16 *x, UInt16 m);
void effect_bandpass(UInt16 *y, volatile UInt16 *x, UInt16 m);
void audioIn_hwi(void);
void audioOut_swi(void);

/* ======== main ======== */
Int main()
{ 
    System_printf("Enter main()\n"); //use ROV->SysMin to view the characters in the circular buffer

    // Set audio_effect function pointer
    audio_effect = &effect_bandpass;
    //audio_effect = &effect_bitCrush;

    //initialization:
    DeviceInit(); //initialize processor

    //jump to RTOS (does not return):
    BIOS_start();
    return(0);
}


/* ======== tickFxn ======== */
//Timer tick function that increments a counter and sets the isrFlag
//Entered 100 times per second if PLL and Timer set up correctly
//Posts the task0's semaphore 20 times per second.
void tickFxn(UArg arg)
{
    tickCount++; //increment the tick counter

    // 20 times per second
    if(tickCount % 5 == 0) {
        // Post semaphore for task0 (gpio_effect_task)
        Semaphore_post(sem0);
    }

    // Twice per second
    if(tickCount % 50 == 0){
        // Tell idle thread to toggle heart-beat LED
        isrFlag = TRUE;
    }
}

/* ======== heartbeatIdleFxn ======== */
// Idle function that is called repeatedly from RTOS
// Toggles on-board LED to indicate that program is running
Void heartbeatIdleFxn(Void)
{
    if(isrFlag == TRUE) {
       isrFlag = FALSE;

       //toggle red LED to indicate program is still running
       GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;
    }
}

/* ======== effect_bitCrush ======== */
// Reduces the resolution of x to the specified
// number of bits (m).
//
// N_bits is the bit resolution of the sample (16- or 12-bit)
//
// Parameters:
// *x - The address of the sample to reduce the resolution.
// m - The desired number of bit resolution.
void effect_bitCrush(UInt16 *y, volatile UInt16 *x, UInt16 m)
{
    // Calculate number of bits to shift by based on UInt16 resolution from ADC
    UInt16 shift = N_bits - m;

    if (shift >= N_bits) shift = 0;

    // Shift right to reduce bit resolution,
    // shift back to original number of bits
    *y = (*x >> shift) << shift;
}

/* ======== effect_echo ======== */
// Adds an echo effect to the sample by
// adding an attenuated sample to the current sample
// with m elements of delay.
//
// Parameters:
// *x - The address of the sample to add delay to.
// m - The amount of delay to add in samples. (order of 1000s of samples)
void effect_echo(UInt16 *y, volatile UInt16 *x, UInt16 m)
{
    float g = 0.25; // This will need to be adjusted by effect knob
    UInt16 delay_i;

    if(m >= buffer_length - buffer_i){
        delay_i = m - (buffer_length - buffer_i);
    }
    else{
        delay_i = buffer_i + m;
    }

    *x = *x + (UInt16)(g*sample_buffer[delay_i]);
    *y = *x;
}


/* ======== effect_chorus ======== */
// Adds a small delay on the order of micro/milli-seconds
// to the current sample to emulate a chorus effect.
// Does not add this to the buffer!
//
// Parameters:
// *x - The address of the sample to add echo to.
// m - The amount of delay to add in samples (order of 10 to 100s of samples)
void effect_chorus(UInt16 *y, volatile UInt16 *x, UInt16 m)
{
    UInt16 delay_i;

    if(m >= buffer_length - buffer_i){
        delay_i = m - (buffer_length - buffer_i);
    }
    else{
        delay_i = buffer_i + m;
    }

    // This should be correct...
    *y = *x + sample_buffer[delay_i];
}

/* ======== effect_bandpass ======== */
// Implements an FIR bandpass filter via Hamming windowing
//
// Parameters:
// *y - The address of the result
// *x - The address of the incoming sample
// m - The amount of echo to add in samples
void effect_bandpass(UInt16 *y, volatile UInt16 *x, UInt16 m)
{
    // Assume our output range will have a frequency spread of 10Hz - 10kHz
    // Want passband width of ~1kHz ... Lowpass prototype width of 500Hz
    // Center frequency is variable in the final "Wah" effect, but for now I'm fixing it at 2kHz
    // Transition width of 2kHz


    // Currently this does not work... We'll need to try testing the lpf first
    UInt16 n;
    UInt16 delay_i;

    for(n = 0; n < N; n++){

        if(n >= buffer_length - buffer_i){
            delay_i = n - (buffer_length - buffer_i);
        }
        else{
            delay_i = buffer_i + n;
        }


        *y += ((Float)sample_buffer[delay_i] * h[n]);
    }

}


/* ======== audioIn_hwi ======== */
// Hardware interrupt for the ADC measuring the
// audio input voltage. Stores result in circular buffer.
void audioIn_hwi(void)
{
    // Store sample sample in the next buffer slot
    sample_buffer[buffer_i] = AdcdResultRegs.ADCRESULT0; //get reading from ADC SOC0

    // Set SWI post indicating new sample has been captured
    Swi_post(audioOut_swi_handle);

    AdcdRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear interrupt flag
}

/* ======== effectIn1_hwi ======== */
// Hardware interrupt for the ADC measuring the
// effect knob output voltage.
void effectIn1_hwi(void){
    UInt16 effect1_result;

    effect1_result = AdccResultRegs.ADCRESULT0; // Get reading from ADC SOC0

    // Clear interrupt flag
    AdccRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;
}

/* ======== audioOut_swi ======== */
// Software interrupt called when a new
// audio sample has been added to the buffer.
// Processes audio sample and outputs result
// on audio output DAC.
void audioOut_swi(void){
    UInt16 y = 0;

    // Calling audio_effect function to perform DSP
    audio_effect(&y, &sample_buffer[buffer_i], 12);

    //DacbRegs.DACVALS.bit.DACVALS = sample_buffer[buffer_i]; // audio pass-through for debug

    // Circular buffer indexing
    if(buffer_i >= buffer_length - 1) buffer_i = 0;
    else buffer_i++;

    // Output on DAC
    DacbRegs.DACVALS.bit.DACVALS = y;
}


/* ======== gpio_effect_task ======== */
// This function is the task that runs periodically to check the gpio
// inputs and change the current effect function based on the input selected.
void gpio_effect_task(void){

    while(TRUE){
        // Wait for semaphore post from timer...
        Semaphore_pend(sem0, BIOS_WAIT_FOREVER);

        // In theory there is no need to wait for Swi to post
        // a semaphore because it will always pre-empt gpio_effect_task
        // and is read-only for the audio_effect function.

        // Check GPIO inputs to see which effect switch is active
        if(GpioDataRegs.GPBDAT.bit.GPIO32) effect_num = 1;//audio_effect = &effect_bandpass;
        else if(GpioDataRegs.GPCDAT.bit.GPIO67) effect_num = 2;//audio_effect = &effect_bitCrush;
        else if(GpioDataRegs.GPDDAT.bit.GPIO111) effect_num = 3; //audio_effect = &effect_chorus;
        else if(GpioDataRegs.GPADAT.bit.GPIO22) effect_num = 4; //audio_effect = &effect_echo;
    }
}
