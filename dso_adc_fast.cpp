/**
 * Derived from https://github.com/pingumacpenguin/STM32-O-Scope/blob/master/STM32-O-Scope.ino
 */
/*.
(c) Andrew Hull - 2015
STM32-O-Scope - aka "The Pig Scope" or pigScope released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
https://github.com/pingumacpenguin/STM32-O-Scope
Adafruit Libraries released under their specific licenses Copyright (c) 2013 Adafruit Industries.  All rights reserved.
*/
/**
 We use PA0 as input pin
 * DMA1, channel 0
 
 * Vref is using PWM mode for Timer4/Channel 3
 * 
 */

#include "dso_global.h"
#include "dso_adc_priv.h"
#include "fancyLock.h"
/**
 */


InterruptStats adcInterruptStats;
uint32_t convTime;
extern HardwareTimer Timer2;
adc_reg_map *adc_Register;



uint16_t DSOADC::adcInternalBuffer[ADC_INTERNAL_BUFFER_SIZE] __attribute__ ((aligned (8)));;;

int dmaSpuriousInterrupt=0;
extern HardwareTimer Timer4;
static bool triggered=false;

/**
 */
int requestedSamples;
uint32_t vcc; // power Supply in mv


FancySemaphore      *dmaSemaphore;
DSOADC             *instance=NULL;

/**
 * 
 */
DSOADC::DSOADC()
{
  instance=this;
  adc_calibrate(ADC1);
  adc_calibrate(ADC2);
 
  // Set up our sensor pin(s)
  pinMode(analogInPin, INPUT_ANALOG);
  dmaSemaphore=new FancySemaphore;  
  adc_Register=  PIN_MAP[analogInPin].adc_device->regs;
  
  setTriggerMode(DSOADC::Trigger_Run);
  
  enableDisableIrq(false);
  enableDisableIrqSource(false,ADC_AWD);
  enableDisableIrqSource(false,ADC_EOC);  
  
  attachWatchdogInterrupt(NULL);
}
 

// Grab the samples from the ADC
// Theoretically the ADC can not go any faster than this.
//
// According to specs, when using 72Mhz on the MCU main clock,the fastest ADC capture time is 1.17 uS. As we use 2 ADCs we get double the captures, so .58 uS, which is the times we get with ADC_SMPR_1_5.
// I think we have reached the speed limit of the chip, now all we can do is improve accuracy.
// See; http://stm32duino.com/viewtopic.php?f=19&t=107&p=1202#p1194


bool DSOADC::startDMASampling (int count)
{
  adcInterruptStats.start();
  if(count>ADC_INTERNAL_BUFFER_SIZE/2)
        count=ADC_INTERNAL_BUFFER_SIZE/2;
  
  requestedSamples=count;  
  convTime=micros();  
  enableDisableIrqSource(false,ADC_AWD);
  enableDisableIrq(true);
  setupAdcDmaTransfer( requestedSamples,adcInternalBuffer, DMA1_CH1_Event );
  return true;
}

/**
 * 
 */
void SPURIOUS_INTERRUPT()
{
    adcInterruptStats.spurious++;
}
/**
 * 
 */
void DSOADC::DMA1_CH1_Event() 
{
    SampleSet one,two;
    adcInterruptStats.adcEOC++;
    adcInterruptStats.eocTriggered++;
    adcInterruptStats.eocIgnored=0;
    
    two.samples=0;
    one.samples=requestedSamples;
    one.data=adcInternalBuffer;
    two.data=NULL;
    instance->captureComplete(one,two);
    adc_dma_disable(ADC1);
}


/**
 */
void DSOADC::captureComplete(SampleSet &one, SampleSet &two)
{
    convTime=micros()-convTime;
    _captured.set1=one;
    _captured.set2=two;
    adcInterruptStats.nbCaptured++;
    dmaSemaphore->giveFromInterrupt();
}


#include "dso_adc_fast_trigger.cpp"
#include "dso_adc_util.cpp"