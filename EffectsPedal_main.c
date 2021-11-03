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
//                      Kieran Bako, A01028276
// Modification Date:   2021-10-13
// Added myIdleFxn2 to print to print to the SysMin OutputBuffer every second.

//defines:
#define xdc__strict //suppress typedef warnings
#define buffer_length 9000
#define fs 48000 // Sampling Frequency - 48kHz
#define L 83 // Hamming window length for tw = 2kHz

//includes:
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <Headers/F2837xD_device.h>
#include <math.h>

//Swi Handle defined in .cfg file:
extern const Swi_Handle audioOut_swi_handle;
extern const Semaphore_Handle task0;

//function prototypes:
extern void DeviceInit(void);

//declare global variables:
volatile Bool isrFlag = FALSE; //flag used by idle function
volatile Bool isrFlag2 = FALSE; // MP - flag used by idle2 function
volatile UInt i = 0; // MP - Seconds counter
volatile Int tickCount = 0; //counter incremented by timer interrupt
volatile float *h_bpf;

/* ---- Declare Buffer ---- */
// Having a buffer (or struct) longer than ~10,000 elements
// throws an error due to how the RAM is addressed
volatile Uint16 sample_buffer[buffer_length];

volatile Uint16 buffer_i = 0; // Current index of buffer

void (*audio_effect)(UInt *, volatile UInt16 *, UInt16); // Declare pointer to function
void effect_bitCrush(UInt16 *y, volatile UInt16 *x, UInt16 m);
void effect_echo(UInt16 *y, volatile UInt16 *x, UInt16 m);
void effect_chorus(UInt16 *y, volatile UInt16 *x, UInt16 m);
void effect_bandpass(UInt16 *y, volatile UInt16 *x, UInt16 m);
float * generate_fir_lpf(float fc, float tw);
float * generate_fir_bpf(float fc, float tw, float fc_lpf);
int calc_fir_len(float tw);
void audioIn_hwi(void);
void audioOut_swi(void);

/* ======== main ======== */
Int main()
{ 
    System_printf("Enter main()\n"); //use ROV->SysMin to view the characters in the circular buffer

    // Set audio_effect function pointer
    audio_effect = &effect_bandpass;
    //audio_effect = &effect_bitCrush;

    float tw = 2000.0;
//    int L = calc_fir_len(tw);
    float fc_lpf = 500.0;
    float fc = 2000.0;

    h_bpf = generate_fir_bpf(fc, tw, fc_lpf);

    //initialization:
    DeviceInit(); //initialize processor

    //jump to RTOS (does not return):
    BIOS_start();
    return(0);
}

//int calc_fir_len(float tw){
//    // Calculate L for Hamming Window
//    float L_f = 3.44 * fs/tw;
//    int L = (int)ceilf(L_f);
//    if (L % 2 == 0) L++;
//
//    return L;
//}

float * generate_fir_lpf(float fc, float tw){
    float omega_c = (2*M_PI)*(fc+tw/2)/fs;
    int n;

    static float h_lpf[L];
    int bias = L/2; // Calculate bias to subtract from n since it starts at 0

    // Calculate h for the FIR lowpass filter
    for(n = 0; n<L; n++) h_lpf[n] = sin((n-bias)*omega_c)/((n-bias)*M_PI);

    //////// NOTE: I'm fairly certain the above line needs to have a bias applied to n due to causality

    return h_lpf;
}

float * generate_fir_bpf(float fc, float tw, float fc_lpf){
    float w_n;
    int n;
    float M_2PI = 2*M_PI; // Pre-calculate 2*pi
    float omega_0 = M_2PI*fc/fs; // Pre-calculate center frequency of BPF
    float *h_lpf;
    static float h_bpf[L];

    h_lpf = generate_fir_lpf(fc_lpf, tw);

    int bias = L/2; // Calculate bias to subtract from n since it starts at 0

    for(n = 0; n < L; n++){
        // Calculate Hamming window
        w_n = 0.54 + 0.46*cos(M_2PI*(float)(n-bias)/(L-1));

        // Calculate h for the FIR bandpass filter
        h_bpf[n] = *(h_lpf+n)*w_n*cos((float)(n-bias)*omega_0);
    }

    return h_bpf;
}

/* ======== myTickFxn ======== */
//Timer tick function that increments a counter and sets the isrFlag
//Entered 100 times per second if PLL and Timer set up correctly
//Posts the task0's semaphore 20 times per second.
void myTickFxn(UArg arg)
{
    tickCount++; //increment the tick counter

    // 20 times per second
    if(tickCount % 5 == 0) {
        //Semaphore_post(task0); // Post semaphore for task0
    }

    // Twice per second
    if(tickCount % 50 == 0){
        isrFlag = TRUE; //tell idle thread to toggle LED
    }
}

/* ======== heartbeatIdleFxn ======== */
// Idle function that is called repeatedly from RTOS
// Toggles on-board LED to indicate that program is running
void heartbeatIdleFxn(Void)
{
   if(isrFlag == TRUE) {
       isrFlag = FALSE;

       //toggle blue LED
       GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
   }
}

/* ======== effect_bitCrush ======== */
// Reduces the resolution of x to the specified
// number of bits (m).
//
// Parameters:
// *x - The address of the sample to reduce the resolution.
// m - The desired number of bit resolution.
void effect_bitCrush(UInt16 *y, volatile UInt16 *x, UInt16 m)
{
    // Calculate number of bits to shift by based on UInt16 resolution from ADC
    UInt16 shift = 16 - m;

    if (shift >= 16) shift = 0;

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

    for(n = 0; n < L; n++){

        if(n >= buffer_length - buffer_i){
            delay_i = n - (buffer_length - buffer_i);
        }
        else{
            delay_i = buffer_i + n;
        }

        *y += (float)sample_buffer[delay_i]*(*(h_bpf + n));
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
    //effect1 = AdcaResultRegs.ADCRESULT0; // Get reading from ADC SOC0

    // Clear interrupt flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
}

/* ======== audioOut_swi ======== */
// Software interrupt called when a new
// audio sample has been added to the buffer.
// Processes audio sample and outputs result
// on audio output DAC.
void audioOut_swi(void){
    UInt16 y = 0;

    // Calling audio_effect function to perform DSP
    audio_effect(&y, &sample_buffer[buffer_i], 13);

    // Circular buffer indexing
    if(buffer_i >= buffer_length - 1) buffer_i = 0;
    else buffer_i++;

    // Output on DAC, shift down to 12-bit resolution
    //DacbRegs.DACVALS.bit.DACVALS = sample_buffer[buffer_i] >> 4; // audio pass-through for debug
    DacbRegs.DACVALS.bit.DACVALS = y >> 4;
}


/* ======== gpio_effect_task ======== */
// This function is the task that runs periodically to check the gpio
// inputs and change the current effect function based on the input selected.
void gpio_effect_task(void){

    // loop forever
    while(TRUE){
        // Wait for semaphore post from timer... COMMENTED OUT FOR NOW
        Semaphore_pend(task0, BIOS_WAIT_FOREVER);
        // Need to also wait for post from Swi... NOT IMPLEMENTED YET

        // Check GPIO inputs to see which effect switch is active
        if(GpioDataRegs.GPBDAT.bit.GPIO32) audio_effect = &effect_bandpass;
        else if(GpioDataRegs.GPCDAT.bit.GPIO67) audio_effect = &effect_bitCrush;
        else if(GpioDataRegs.GPDDAT.bit.GPIO111) audio_effect = &effect_chorus;
        else if(GpioDataRegs.GPADAT.bit.GPIO22) audio_effect = &effect_echo;
    }
}
