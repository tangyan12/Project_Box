#include "stm32f10x.h"

#define SG90_PWM_PORT GPIOB
#define SG90_PWM_PIN GPIO_Pin_1

void SG90_GPIO_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    /*如果要使用重映像，要开启CRC的时钟*/
    // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);
    // GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3,ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=SG90_PWM_PIN;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(SG90_PWM_PORT,&GPIO_InitStructure);
}

#define SG90_PWM_Period 1000
#define SG90_PWM_Prescaler 1440
#define SG90_TIMER TIM3
#define SG90_PWM_T 20

void SG90_PWM_Init(void)
{
    SG90_GPIO_Init();

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);

    TIM_InternalClockConfig(SG90_TIMER);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseStructInit(&TIM_TimeBaseInitStructure);
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period=SG90_PWM_Period-1;
    TIM_TimeBaseInitStructure.TIM_Prescaler=SG90_PWM_Prescaler-1;
    TIM_TimeBaseInit(SG90_TIMER,&TIM_TimeBaseInitStructure);

    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;//SG90要用PWM1
    TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse=0;
    /*如果更改了通道，这里也要记得更改*/
    TIM_OC4Init(SG90_TIMER,&TIM_OCInitStructure);

    TIM_Cmd(SG90_TIMER,ENABLE);

}

typedef enum {
    SG90_Angle_F_90=25,
    SG90_Angle_F_45=50,
    SG90_Angle_0=75,
    SG90_Angle_45=100,
    SG90_Angle_90=125,
}SG90_PWM_Angle ;

void SG90_Turn_Angle( SG90_PWM_Angle Angle)
{
    /*如果更改了通道，这里也要记得更改*/
    TIM_SetCompare4(SG90_TIMER,Angle);
}
