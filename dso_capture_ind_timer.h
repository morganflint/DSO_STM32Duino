
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
static int transformTimer(int16_t *in, float *out,int count, VoltageSettings *set,int expand,CaptureStats &stats, float triggerValue, DSOADC::TriggerMode mode)
{
   if(!count) return false;
   stats.xmin=200;
   stats.xmax=-200;
   stats.avg=0;
   int ocount=(count*4096)/expand;
   if(ocount>240)
   {
       ocount=240;
   }
   ocount&=0xffe;
   int dex=0;
   
   // First
   float f;
   {
       f=(float)in[0]; 
       f-=set->offset;
       f*=set->multiplier;       
       if(f>stats.xmax) stats.xmax=f;
       if(f<stats.xmin) stats.xmin=f;       
       out[0]=f; // Unit is now in volt
       stats.avg+=f;
       dex+=expand;
   }
   
   // med
   //if(stats.trigger==-1)
   {   
    for(int i=1;i<ocount;i++)
    {

        f=*(in+2*(dex/4096));
        f-=set->offset;
        f*=set->multiplier;
        if(f>stats.xmax) stats.xmax=f;
        if(f<stats.xmin) stats.xmin=f;       
        out[i]=f; // Unit is now in volt

        if(stats.trigger==-1)
        {
             if(mode!=DSOADC::Trigger_Rising)
                 if(out[i-1]<triggerValue&&out[i]>=triggerValue) stats.trigger=i;
             if(mode!=DSOADC::Trigger_Falling)
                 if(out[i-1]>triggerValue&&out[i]<=triggerValue) stats.trigger=i;
        }

        stats.avg+=f;
        dex+=expand;
    }   
   }
   stats.avg/=count;
   return ocount;
}

/**
 * 
 * @return 
 */
bool DSOCapture::prepareSamplingTimer()
{
    return adc->prepareTimerSampling(timerBases[currentTimeBase].fq);
}

/**
 * 
 * @return 
 */
DSOCapture::DSO_TIME_BASE DSOCapture::getTimeBaseTimer()
{
    return (DSOCapture::DSO_TIME_BASE)(currentTimeBase+DSO_TIME_BASE::DSO_TIME_BASE_5MS);
}



/**
 * 
 */
void DSOCapture::stopCaptureTimer()
{
    adc->stopTimeCapture();     
}


/**
 * 
 * @return 
 */

const char *DSOCapture::getTimeBaseAsTextTimer()
{
    return timerBases[currentTimeBase].name;
}

/**
 * 
 * @param count
 * @return 
 */
bool       DSOCapture:: startCaptureTimer (int count)
{    
    return adc->startTimerSampling(count);
}
/**
  * 
 */
bool       DSOCapture:: startCaptureTimerTrigger (int count)
{
    return adc->startTriggeredTimerSampling(count,triggerValueADC);
}
/**
 */
bool DSOCapture::taskletTimer()
{
    xDelay(20);
    FullSampleSet fset; // Shallow copy
    int16_t *p;
    while(1)
    {
        int currentVolt=currentVoltageRange; // use a local copy so that it does not change in the middle
        int currentTime=currentTimeBase;      
        if(!adc->getSamples(fset))
            continue;
        if(!fset.set1.samples)
        {
            continue;
        }
        
        CapturedSet *set=captureSet;
        set->stats.trigger=-1;
        int scale=vSettings[currentVolt].inputGain;
        int expand=4096;
        
        float *data=set->data;    
        if(fset.shifted)
            p=((int16_t *)fset.set1.data)+1;
        else
            p=((int16_t *)fset.set1.data);

        set->samples=transformTimer(
                                        p,
                                        data,
                                        fset.set1.samples,
                                        vSettings+currentVolt,
                                        expand,
                                        set->stats,
                                        triggerValueFloat,
                                        adc->getTriggerMode());
        if(fset.set2.samples)
        {
            CaptureStats otherStats;
            if(fset.shifted)
                p=((int16_t *)fset.set2.data)+1;
            else
                p=((int16_t *)fset.set2.data);
            int sample2=transformTimer(
                                        p,
                                        data+set->samples,
                                        fset.set2.samples,
                                        vSettings+currentVolt,
                                        expand,
                                        otherStats,
                                        triggerValueFloat,
                                        adc->getTriggerMode());                
            if(set->stats.trigger==-1 && otherStats.trigger!=-1) 
            {
                set->stats.trigger=otherStats.trigger+set->samples;
            }            
            set->stats.avg= (set->stats.avg*set->samples+otherStats.avg*fset.set2.samples)/(set->samples+fset.set2.samples);
            set->samples+=sample2;
            if(otherStats.xmax>set->stats.xmax) set->stats.xmax=otherStats.xmax;
            if(otherStats.xmin<set->stats.xmin) set->stats.xmin=otherStats.xmin;
            
        }
        set->stats.frequency=-1;
        
        float f=computeFrequency(fset.shifted,fset.set1.samples,fset.set1.data);
        f=((float)timerBases[currentTimeBase].fq)*1000./f;
        set->stats.frequency=f;
        
        // Data ready!
        captureSemaphore->give();
    }
}