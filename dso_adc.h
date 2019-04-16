/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/
#pragma once
#include <Wire.h>


// Sampling Queue

#define ADC_INTERNAL_BUFFER_SIZE 1024
/* 
 * \brief Describe a voltage setting
 */

typedef struct VoltageSettings
{
    const char *name;           /// name of the setting i.e 10 ms/div
    int         inputGain;      /// Input gain selected 4051/4053 switch 
    float       displayGain;    /// multiply by this to get pixels from volt
    float       multiplier;     /// Gain of the internal amplifier, multiply by this to get volts
    int         offset;         /// Offset of sampled data , you need to substract it

};

typedef struct TimeSettings
{
  const char    *name;
  adc_prescaler  prescaler;
  adc_smp_rate   rate;
  int            expand4096;
};

typedef struct SampleSet
{
  int       samples;
  uint32_t  *data;
};

static bool triggered=false;




/**
 * \class DSOADC
 * \brief wrapper on top of the DMA-ADC engine
 */
class DSOADC
{
public:  
  enum TriggerMode
  {
    Trigger_Rising,
    Trigger_Falling,
    Trigger_Both,
  };
  
  
public:
                    DSOADC();
            bool    setTimeScale(adc_smp_rate one, adc_prescaler two);
            bool    prepareDMASampling (adc_smp_rate rate,adc_prescaler scale);
            bool    prepareTimerSampling (int fq);
            bool    getSamples(SampleSet &set1,SampleSet &set2)           ;
            bool     setSlowMode(int fqInHz);
            bool     readCalibrationValue();
    static  uint32_t getVCCmv();
            bool     setTriggerMode(TriggerMode mode);
            bool     getTriggerState();
            bool     setVrefPWM(int ratio); // Trigger 
            

            bool startDMASampling (int count);
            bool startTimerSampling (int count);
            bool startTriggeredTimerSampling (int count,uint32_t triggerADC);
            void clearCapturedData();
protected:            
            void setADCs ();
    static  void adc_dma_disable(const adc_dev * dev) ;            
    static  void adc_dma_enable(const adc_dev * dev) ;    
    static  void DMA1_CH1_Event();
            void captureComplete(SampleSet &one, SampleSet &two);
    static  void Timer2_Event();
    static  void Timer2Trigger_Event();

            void timerCapture();
            void timerTriggerCapture();
            
            bool startInternalDmaSampling ();
            
            bool validateAverageSample(uint32_t &avg);
    static void TriggerInterrupt();
            
            

protected:
  
            int             _sampled;
            SampleSet       _cap1,_cap2;
 
static      uint32_t adcInternalBuffer[ADC_INTERNAL_BUFFER_SIZE];            
};

// EOF

