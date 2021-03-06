/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/
#include "dso_global.h"
#include "dso_adc.h"
#include "dso_capture.h"
#include "dso_capture_priv.h"
#include "dso_adc_gain.h"
#include "DSO_config.h"
/**
 * 
 * @param in
 * @param out
 * @param count
 * @param set
 * @param expand
 * @param stats
 * @param triggerValue
 * @param mode
 * @return 
 */
static int transformTimer(int dc0_ac1,int16_t *in, float *out,int count, VoltageSettings *set,int expand,CaptureStats &stats,int swing)
{
   if(!count) return false;
   stats.xmin=200;
   stats.xmax=-200;
   stats.avg=0;
   stats.saturation=false;
   int ocount=(count*4096)/expand;
   if(ocount>240)
   {
       ocount=240;
   }
   ocount&=0xffe;
   
    float offset,multiplier;   
    offset=DSOInputGain::getOffset(dc0_ac1);
    multiplier=DSOInputGain::getMultiplier();

   
    for(int i=0;i<ocount;i++)
    {
        int v=in[i];
        if(v<swing) stats.saturation=true;
        if(v>(4096-swing)) stats.saturation=true;
        float f=(float)v;
        f-=offset;
        f*=multiplier;
        if(f>stats.xmax) stats.xmax=f;
        if(f<stats.xmin) stats.xmin=f;       
        out[i]=f; // Unit is now in volt
        stats.avg+=f;
        
    }   
   stats.avg/=count;
   return ocount;
}

/**
 * 
 * @return 
 */
bool DSOCapturePriv::prepareSamplingTimer()
{
    return adc->prepareTimerSampling(timerBases[currentTimeBase].fq);
}

/**
 * 
 * @return 
 */
DSOCapture::DSO_TIME_BASE DSOCapturePriv::getTimeBaseTimer()
{
    return (DSOCapture::DSO_TIME_BASE)(currentTimeBase+DSO_TIME_BASE::DSO_TIME_BASE_5MS);
}



/**
 * 
 */
void DSOCapturePriv::stopCaptureTimer()
{
    adc->stopTimeCapture();     
}


/**
 * 
 * @return 
 */

const char *DSOCapturePriv::getTimeBaseAsTextTimer()
{
    return timerBases[currentTimeBase].name;
}

//--
bool        DSOCapturePriv::nextCaptureTimer(int count)
{
    return startCaptureTimer(count);
}
bool        DSOCapturePriv::nextCaptureTimerTrigger(int count)
{
    return startCaptureTimerTrigger(count);
}
/**
 * 
 * @param count
 * @return 
 */
bool       DSOCapturePriv:: startCaptureTimer (int count)
{    
    return adc->startTimerSampling(count);
}
/**
  * 
 */
bool       DSOCapturePriv:: startCaptureTimerTrigger (int count)
{
    return adc->startTriggeredTimerSampling(count,triggerValueADC);
}
/**
 */
bool DSOCapturePriv::taskletTimer()
{    
    FullSampleSet fset; // Shallow copy
    int16_t *p;

    int currentVolt=currentVoltageRange; // use a local copy so that it does not change in the middle
    
    if(!adc->getSamples(fset))
        return false;
    if(!fset.set1.samples)
    {
        return false;
    }

    CapturedSet *set=captureSet;            
    int expand=4096;

    float *data=set->data;    
    p=((int16_t *)fset.set1.data);
    set->samples=transformTimer(    INDEX_AC1_DC0(),
                                    p,
                                    data,
                                    fset.set1.samples,
                                    vSettings+currentVolt,
                                    expand,
                                    set->stats,
                                    vSettings[DSOCapturePriv::currentVoltageRange].maxSwing);
    if(fset.set2.samples)
    {
        CaptureStats otherStats;
        p=((int16_t *)fset.set2.data);
        int sample2=transformTimer( INDEX_AC1_DC0(),
                                    p,
                                    data+set->samples,
                                    fset.set2.samples,
                                    vSettings+currentVolt,
                                    expand,
                                    otherStats,
                                    vSettings[DSOCapturePriv::currentVoltageRange].maxSwing);                
        set->stats.avg= (set->stats.avg*set->samples+otherStats.avg*fset.set2.samples)/(set->samples+fset.set2.samples);
        set->samples+=sample2;
        if(otherStats.xmax>set->stats.xmax) set->stats.xmax=otherStats.xmax;
        if(otherStats.xmin<set->stats.xmin) set->stats.xmin=otherStats.xmin;

    }
    set->stats.frequency=-1;
    if(fset.set1.samples>100) // need a minimum set of samples
    {
        int fint=computeFrequency(fset.set1.samples,fset.set1.data);
        if(fint)
        {
            float f=fint;
            f=((float)timerBases[currentTimeBase].fq)*1000./f;
            set->stats.frequency=f;
            set->stats.trigger=120;
        }
    }

    // Data ready!
    captureSemaphore->give();
    return true;
}