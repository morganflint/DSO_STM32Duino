/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/
#include "dso_includes.h"
#include "Fonts/Targ56.h"
#include "Fonts/digitLcd56.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "gfx/dso_small_compressed.h"


extern void  menuManagement(void);

extern void splash(void);
static void drawGrid(void);

//--
extern Adafruit_TFTLCD_8bit_STM32 *tft;
extern DSOControl *controlButtons;
extern testSignal *myTestSignal;
extern DSOADC   *adc;
//
extern float test_samples[256];
static uint8_t waveForm[256]; // take a bit more, we have rounding issues

uint32_t  refrshDuration=0;
int       nbRefrsh=0;

static    int lastTrigger=-1;
static    DSOControl::DSOCoupling oldCoupling;

static void initMainUI(void);
void drawBackground();
/**
 * 
 */
void splash(void)
{
        tft->fillScreen(BLACK);   
        tft->drawRLEBitmap(dso_small_width,dso_small_height,20,64,WHITE,BLACK,dso_small);
        tft->setFontSize(Adafruit_TFTLCD_8bit_STM32::SmallFont);        
        
        tft->setTextColor(WHITE,BLACK);        
        tft->setCursor(140, 64);
        tft->myDrawString("DSO-STM32duino");              
        tft->setCursor(140, 84);
#ifdef USE_RXTX_PIN_FOR_ROTARY        
        tft->myDrawString("USB  Version");              
#else
        tft->myDrawString("RXTX Version");              
#endif
        char bf[20];
        sprintf(bf,"%d.%02d",DSO_VERSION_MAJOR,DSO_VERSION_MINOR);
        tft->setCursor(140, 64+20*2);
        tft->myDrawString(bf);              
}

 

static void redraw()
{
        DSODisplay::drawGrid();
        DSODisplay::printVoltTimeTriggerMode(capture->getVoltageRangeAsText(), capture->getTimeBaseAsText(),DSOCapture::getTriggerMode());
        DSODisplay::printTriggerValue(DSOCapture::getTriggerValue());
        DSODisplay::printOffset(capture->getVoltageOffset());
}
#define STOP_CAPTURE() DSOCapture::stopCapture()

static void buttonManagement()
{
    bool dirty=false;
        
    DSOControl::DSOCoupling coupling=controlButtons->getCouplingState();
    if(coupling!=oldCoupling)
    {
        oldCoupling=coupling;
        dirty=true;
    }
    
    if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_OK) & EVENT_LONG_PRESS)    
    {
        STOP_CAPTURE();
        menuManagement();
        drawBackground();
        
        return;
    }
    int inc=controlButtons->getRotaryValue();
    
    DSODisplay::MODE_TYPE newMode=DSODisplay::INVALID_MODE;
    
    if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_VOLTAGE) & EVENT_SHORT_PRESS)
    {
        dirty=true;
        newMode=DSODisplay::VOLTAGE_MODE;        
    }
    if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_TIME) & EVENT_SHORT_PRESS)
    {
        dirty=true;
        newMode=DSODisplay::TIME_MODE;
    }
    if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_TRIGGER) & EVENT_SHORT_PRESS)
    {
        dirty=true;
        newMode=DSODisplay::TRIGGER_MODE;
    }

    if(dirty)
    {
        if((DSODisplay::getMode()&0x7f)==newMode) // switch between normal & alternate
            newMode=(DSODisplay::MODE_TYPE)(DSODisplay::getMode()^0x80);
        DSODisplay::setMode(newMode);
        redraw();
    }
    dirty=false;

    if(inc)
    {

        switch(DSODisplay::getMode())
        {
            case DSODisplay::VOLTAGE_MODE: 
                {
                int v=capture->getVoltageRange();
                    dirty=true;
                v+=inc;
                if(v<0) v=0;
                if(v>DSOCapture::DSO_VOLTAGE_MAX) v=DSOCapture::DSO_VOLTAGE_MAX;
                capture->setVoltageRange((DSOCapture::DSO_VOLTAGE_RANGE)v);                                
                STOP_CAPTURE();
                }
                break;
            case DSODisplay::TIME_MODE: 
                {
                    int v=capture->getTimeBase();
                        dirty=true;
                    v+=inc;
                   if(v<0) v=0;
                   if(v>DSOCapture::DSO_TIME_BASE_MAX) v=DSOCapture::DSO_TIME_BASE_MAX;
                   DSOCapture::DSO_TIME_BASE  t=(DSOCapture::DSO_TIME_BASE )v;
                   DSOCapture::clearCapturedData();
                   capture->setTimeBase( t);
                   STOP_CAPTURE();
                }
                break;
            case DSODisplay::TRIGGER_MODE: 
                {
                    int t=capture->getTriggerMode();
                    t+=inc;
                    while(t<0) t+=4;
                    t%=4;
                    capture->setTriggerMode((DSOCapture::TriggerMode)t);                   
                    capture->setTimeBase( capture->getTimeBase()); // this will refresh the internal indirection table
                    dirty=true;
                }
                break;
            case DSODisplay::VOLTAGE_MODE_ALT:
            {
                float v=capture->getVoltageOffset();
                v+=0.1*inc;
                capture->setVoltageOffset(v);                
                dirty=true;
                STOP_CAPTURE();
                break;
            }
            case DSODisplay::TRIGGER_MODE_ALT: 
                {
                 float v=capture->getTriggerValue();

                    v+=0.1*(float)inc;
                   capture->setTriggerValue(v);    
                   dirty=true;
                   STOP_CAPTURE();
                }
                break;
            default: 
                            break;
        }       
    if(dirty)
        redraw();
    }
}
void drawBackground()
{
    tft->fillScreen(BLACK);
    tft->setFontSize(Adafruit_TFTLCD_8bit_STM32::SmallFont);   
    tft->setTextSize(2);
    DSODisplay::drawGrid();
    DSODisplay::drawStatsBackGround();
    DSODisplay::printVoltTimeTriggerMode(capture->getVoltageRangeAsText(), capture->getTimeBaseAsText(),DSOCapture::getTriggerMode());
    DSODisplay::printTriggerValue(DSOCapture::getTriggerValue());
    DSODisplay::printOffset(capture->getVoltageOffset());
   

}
/**
 * 
 */
void initMainUI(void)
{
    DSOCapture::setTimeBase(    DSOCapture::DSO_TIME_BASE_5MS);
    DSOCapture::setVoltageRange(DSOCapture::DSO_VOLTAGE_1V);
    DSOCapture::setTriggerValue(1.);
    drawBackground();    
    
    float f=DSOCapture::getTriggerValue();
    DSODisplay::printTriggerValue(f);
    
    lastTrigger=DSOCapture::voltageToPixel(f);
    DSODisplay::drawVoltageTrigger(true,lastTrigger);
    
}

/**
 * 
 */
void mainDSOUI(void)
{
    CaptureStats stats;    

    int triggerLine;
    
    DSODisplay::init();
    initMainUI();
   
    uint32_t lastRefresh=millis();

    float f=DSOCapture::getTriggerValue();
    triggerLine=DSOCapture::voltageToPixel(f);
    DSOControl::DSOCoupling oldCoupling=controlButtons->getCouplingState();
    while(1)
    {        
        int count;  
        
        count=DSOCapture::triggeredCapture(240,test_samples,stats);  
        
        // Nothing captured, refresh screen
        if(!count) 
        {
            DSODisplay::drawVoltageTrigger(false,triggerLine);
            buttonManagement();
            float f=DSOCapture::getTriggerValue()+DSOCapture::getVoltageOffset();
            triggerLine=DSOCapture::voltageToPixel(f);
            DSODisplay::drawVoltageTrigger(true,triggerLine);
            continue;
        }
        DSOCapture::captureToDisplay(count,test_samples,waveForm);  
        // Remove trigger
        DSODisplay::drawVoltageTrigger(false,triggerLine);
        
        DSODisplay::drawWaveForm(count,waveForm);
        
        
        if(lastTrigger!=-1)
        {
             DSODisplay::drawVerticalTrigger(false,lastTrigger);
             lastTrigger=-1;
        }
        
        if(stats.trigger!=-1)
        {
            lastTrigger=stats.trigger;
            DSODisplay::drawVerticalTrigger(true,lastTrigger);
        }
        float f=DSOCapture::getTriggerValue()+DSOCapture::getVoltageOffset();
        triggerLine=DSOCapture::voltageToPixel(f);
        DSODisplay::drawVoltageTrigger(true,triggerLine);
        
        uint32_t m=millis();
        if(m<lastRefresh)
        {
            m=lastRefresh+101;
        }
        if((m-lastRefresh)>250)
        {
            lastRefresh=m;
            DSODisplay::drawStats(stats);
            refrshDuration+=(millis()-m);
            nbRefrsh++;
        }
        buttonManagement();                
      
    }
        
} 


// EOF    

