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
/**
 */
typedef struct TimeSettings
{
  const char    *name;
  adc_prescaler  prescaler;
  adc_smp_rate   rate;
  int            expand4096;
  int            fqInHz;
};
/**
 * 
 */
class SampleSet
{
public:
            SampleSet()
            {
              samples=0;
              data=NULL;
            }
            void set(int s, uint32_t *d)
            {
              samples=s;
              data=d;
            }
            
            SampleSet(int s, uint32_t *d)
            {
              set(s,d);
            }
  int       samples;
  uint32_t  *data;
};

/**
 */
typedef struct FullSampleSet
{
  bool      shifted;
  SampleSet set1;
  SampleSet set2;
  
};


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
            bool    getSamples(FullSampleSet &fullSet)           ;
            bool     setSlowMode(int fqInHz);
            bool     readCalibrationValue();
    static  uint32_t getVCCmv();
            bool     setTriggerMode(TriggerMode mode);
            TriggerMode getTriggerMode() {return _triggerMode;};
            bool     getTriggerState();
            bool     setVrefPWM(int ratio); // Trigger 
            

            bool startDMASampling (int count);
            bool startDMATriggeredSampling (int count, int ADCTriggerValue);
            bool startTimerSampling (int count);
            bool startTriggeredTimerSampling (int count,uint32_t triggerADC);

    static  void adc_dma_disable(const adc_dev * dev) ;            
    static  void adc_dma_enable(const adc_dev * dev) ;    

            
protected:            
            void setADCs ();
    static  void DMA1_CH1_Event();
    static  void DMA1_CH1_TriggerEvent() ;
            void captureComplete(bool shift,SampleSet &one, SampleSet &two);
    static  void Timer_Event();
    static  void Timer_Trigger_Event();

            void timerCapture();
            void timerTriggerCapture();
            
            bool startInternalDmaSampling ();
            
            bool validateAverageSample(uint32_t &avg);
    static void TriggerInterrupt();
public:        
    static void setupAdcDmaTransfer(   int count,uint32_t *buffer, void (*handler)(void) );
    static void nextAdcDmaTransfer( int count,uint32_t *buffer);

    static void enableDisableIrqSource(bool onoff, int interruptMask);
    static void enableDisableIrq(bool onoff);
    static void defaultAdcIrqHandler();
    static void attachWatchdogInterrupt(voidFuncPtr handler);
    static void setWatchdogTriggerValue(uint32_t high, uint32_t low);
    static void watchDogInterrupt();
            void awdTrigger() ;                       
            bool awdTriggered() {return _triggered;}
            void stopDmaCapture();
            void stopTimeCapture();
protected:
  
            int             _sampled;
            FullSampleSet   _captured;
            TriggerMode     _triggerMode;
            bool            _triggered;
static      uint32_t adcInternalBuffer[ADC_INTERNAL_BUFFER_SIZE];            
};

// EOF

