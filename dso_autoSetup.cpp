/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/
#include "dso_includes.h"
extern float test_samples[256];
/**
 * 
 */
void        autoSetup()
{
    // swith to free running mode
    DSOCapture::stopCapture();
    DSOCapture::setTriggerMode(DSOCapture::Trigger_Run);
    
    DSOCapture::DSO_TIME_BASE timeBase=DSOCapture::DSO_TIME_BASE_5MS;
    int voltage=DSOCapture::DSO_VOLTAGE_5V;
    
    DSOCapture::setTimeBase(timeBase);
    DSOCapture::setVoltageRange((DSOCapture::DSO_VOLTAGE_RANGE)voltage);
    
    CaptureStats stats;
    while(1)
    {
        int n=DSOCapture::capture(240,test_samples,stats);
        if(!n)
            continue;
        
        int  xmin= DSOCapture::voltToADCValue(stats.xmin)-2048;
        int  xmax= DSOCapture::voltToADCValue(stats.xmax)-2048;
        
        xmin=abs(xmin);
        xmax=abs(xmax);
        if(xmin>xmax) xmax=xmin;
        
        if(xmax>1900 && voltage<DSOCapture::DSO_VOLTAGE_MAX) // saturation            
        {
            voltage=voltage+1;
            DSOCapture::setVoltageRange((DSOCapture::DSO_VOLTAGE_RANGE)voltage);
            continue;
        }
        if(xmax<1000 && voltage>0) // too small
        {
            voltage=voltage-1;
            DSOCapture::setVoltageRange((DSOCapture::DSO_VOLTAGE_RANGE)voltage);
            continue;
        }
        break;        
    }
    DSOCapture::stopCapture();    
}
