#ifndef __MYDELAY_H__
#define __MYDELAY_H__
#include "stm32f10x.h"

void delay_init(u8 SYSCLK);
void delay_us(u32 nus);
void delay_ms(u32 nms);
void delay_xms(u32 nms);
#endif
