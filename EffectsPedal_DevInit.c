// Filename:            EffectsPedal_DevInit.c
//
// Description:	        Initialization code for Effects Pedal project.
//
// Version:             1.0
//
// Target:              TMS320F28379D
//
// Authors:             Matthew Peeters, A01014378
//                      Kieran Bako, A01028276
//
// Date:   2021-11-17

#include <Headers/F2837xD_device.h>

extern void DelayUs(Uint16);

void DeviceInit(void)
{
EALLOW;
    //initialize GPIO lines:
    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0; //D9 (red LED)
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1; // Output
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1; // Turn on RED LED

    // GPIO32 - Pin 2
    GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 0; // Input

    // GPIO67 - Pin 5
    GpioCtrlRegs.GPCGMUX1.bit.GPIO67 = 0;
    GpioCtrlRegs.GPCDIR.bit.GPIO67 = 0; // Input

    // GPIO111 - Pin 6
    GpioCtrlRegs.GPDGMUX1.bit.GPIO111 = 0;
    GpioCtrlRegs.GPDDIR.bit.GPIO111 = 0; // Input

    // GPIO22 - Pin 8
    GpioCtrlRegs.GPAGMUX2.bit.GPIO22 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 0; // Input

    // GPIO0 - Pin 40
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1; // Output



    //---------------------------------------------------------------
    // INITIALIZE D-A ---- ENSURE PIN 29 IS TIED TO 3.3V (VREFHIB)
    //---------------------------------------------------------------
    CpuSysRegs.PCLKCR16.bit.DAC_B = 1; // Enable DAC clock
    DacbRegs.DACCTL.bit.DACREFSEL = 1; // Set DACREFSEL to VREFHIB/VSSA
    DacbRegs.DACOUTEN.bit.DACOUTEN = 1; // Power up DAC_B (pin 70)

    //---------------------------------------------------------------
    // INITIALIZE A-D ---- AUDIO IN
    //---------------------------------------------------------------
    CpuSysRegs.PCLKCR13.bit.ADC_D = 1; //enable A-D clock for ADC-D
    AdcdRegs.ADCCTL2.bit.PRESCALE = 0x0; // Clock prescale = 1.0
    AdcdRegs.ADCCTL2.bit.SIGNALMODE = 0; // fixed-mode
    AdcdRegs.ADCCTL2.bit.RESOLUTION = 1; // 16-bit resolution
    AdcdRegs.ADCCTL1.bit.ADCPWDNZ = 1;

    //generate INT pulse on end of conversion:
    AdcdRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    //wait 1 ms after power-up before using the ADC:
    DelayUs(1000);

    AdcdRegs.ADCSOC0CTL.bit.TRIGSEL = 2; //trigger source = CPU1 Timer 1
    AdcdRegs.ADCSOC0CTL.bit.CHSEL = 0; // set SOC0 to sample D0
    AdcdRegs.ADCSOC0CTL.bit.ACQPS = 139; //set SOC0 window to 139 SYSCLK cycles
    AdcdRegs.ADCINTSEL1N2.bit.INT1SEL = 0; //connect interrupt ADCINT1 to EOC0
    AdcdRegs.ADCINTSEL1N2.bit.INT1E = 1; //enable interrupt ADCINT1

    //---------------------------------------------------------------
    // INITIALIZE A-D ---- EFFECT KNOB (Pin 27)
    //---------------------------------------------------------------
    CpuSysRegs.PCLKCR13.bit.ADC_C = 1; //enable A-D clock for ADC-C
    AdccRegs.ADCCTL2.bit.PRESCALE = 0x0; // Clock prescale = 1.0
    AdccRegs.ADCCTL2.bit.SIGNALMODE = 0; // fixed mode
    AdccRegs.ADCCTL2.bit.RESOLUTION = 0; // 12-bit resolution
    AdccRegs.ADCCTL1.bit.ADCPWDNZ = 1;

    //generate INT pulse on end of conversion:
    AdccRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    //wait 1 ms after power-up before using the ADC:
    DelayUs(1000);

    AdccRegs.ADCSOC0CTL.bit.TRIGSEL = 1; //trigger source = CPU1 Timer 0 - Effect switch timer (100Hz)
    AdccRegs.ADCSOC0CTL.bit.CHSEL = 2; // set SOC0 to sample C2 (pin 27, vector ID 201)
    AdccRegs.ADCSOC0CTL.bit.ACQPS = 139; //set SOC0 window to 139 SYSCLK cycles
    AdccRegs.ADCINTSEL1N2.bit.INT2SEL = 0; //connect interrupt ADCINT1 to EOC0
    AdccRegs.ADCINTSEL1N2.bit.INT2E = 1; //enable interrupt ADCINT1


EDIS;
}
