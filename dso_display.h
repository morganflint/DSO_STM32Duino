/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/

#pragma once

/**
 */
class DSODisplay
{
public:
            static void  init();
            static void  drawWaveForm(int count,const uint8_t *data);
            static void  drawGrid(void);
            static void  drawVerticalTrigger(bool drawOrErase,int column);
  
  
};