// Filename:            HwiExample_DeviceInit.c
//
// Description:	        Initialization code for Hwi Example
//
// Version:             1.0
//
// Target:              TMS320F28379D
//
// Author:              David Romalo
//
// Date:                19Oct2021

#include <Headers/F2837xD_device.h>

extern void DelayUs(Uint16);

void DeviceInit(void)
{
EALLOW;
    //initialize GPIO lines:
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0; //D10 (blue LED)
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;
    GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;

    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0; //D9 (red LED)
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;
    GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;

    //---------------------------------------------------------------
    // INITIALIZE A-D
    //---------------------------------------------------------------
    CpuSysRegs.PCLKCR13.bit.ADC_A = 1; //enable A-D clock for ADC-A
    AdcaRegs.ADCCTL2.bit.PRESCALE = 0x0;
    AdcaRegs.ADCCTL2.bit.SIGNALMODE = 1; // differential mode
    AdcaRegs.ADCCTL2.bit.RESOLUTION = 1; // 16-bit resolution
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;

    //generate INT pulse on end of conversion:
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    //wait 1 ms after power-up before using the ADC:
    DelayUs(1000);

//    AnalogSubsysRegs.TSNSCTL.bit.ENABLE = 1; //connect temp sensor to ADCIN13 on ADC-A


    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 2; //trigger source = CPU1 Timer 1
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0; //set SOC0 to sample A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 139; //set SOC0 window to 139 SYSCLK cycles
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0; //connect interrupt ADCINT1 to EOC0
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1; //enable interrupt ADCINT1
EDIS;
}
