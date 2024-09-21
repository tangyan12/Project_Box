#include "stm32f10x.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#define USART_X           USART3
#define USART_X_CLOCK     RCC_APB1Periph_USART3 
#define USART_X_PORT	  GPIOB
#define USART_X_PORT_CLK  RCC_APB2Periph_GPIOB
#define USART_X_RXD       GPIO_Pin_11
#define USART_X_TXD       GPIO_Pin_10

#define USART_X_IRQ 	  USART3_IRQn
#define USART_X_IT_PRIORITY 3


void MY_UART3_Init(uint32_t baud)
{
	RCC_APB1PeriphClockCmd(USART_X_CLOCK,ENABLE);
	RCC_APB2PeriphClockCmd(USART_X_PORT_CLK,ENABLE);	
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=USART_X_TXD;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(USART_X_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin=USART_X_RXD;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(USART_X_PORT,&GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate=baud;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode=USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity=USART_Parity_No;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART_X,&USART_InitStructure);
	
	USART_ITConfig(USART_X,USART_IT_RXNE,ENABLE);
	USART_ClearFlag(USART_X,USART_FLAG_RXNE);
	USART_ITConfig(USART_X,USART_IT_IDLE,ENABLE);
	USART_ClearFlag(USART_X,USART_FLAG_IDLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel=USART_X_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=USART_X_IT_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&NVIC_InitStructure);
	
    
	USART_Cmd(USART_X,ENABLE);

}



#define USART3_RX_MAX_SIZE 40
uint16_t USART3_RX_STA;
uint8_t USART3_RX_BUF[USART3_RX_MAX_SIZE];

void USART3_IRQHandler(void)
{
	
    if (USART_GetITStatus(USART3,USART_IT_RXNE)==SET)
    {
        if (!(USART3_RX_STA & (1<<15)))
		{
			USART3_RX_BUF[(USART3_RX_STA&0x7FFF)]=USART_ReceiveData(USART3);
			USART3_RX_STA++;
			USART3_RX_BUF[USART3_RX_STA&0x7FFF]='\0';
			if ((USART3_RX_STA&0x7FFF)>=USART3_RX_MAX_SIZE-1)
			{
				USART3_RX_STA |= (1<<15);				
				
			}	
		}
    	USART_ClearITPendingBit(USART3,USART_IT_RXNE);
    }
	if (USART_GetITStatus(USART3,USART_IT_IDLE)==SET)
	{
		USART3_RX_STA |= (1<<15);
		USART_ReceiveData(USART3);
		USART_ClearFlag(USART3,USART_FLAG_IDLE);
	}
	
	
}
#define DATA_MAX_SIZE 100

void u3_printf(char *format,...)
{
	
	char DataBuf[DATA_MAX_SIZE];
    va_list start;
    va_start(start,format);
    vsprintf(DataBuf,format,start);
    va_end(start);

    uint8_t pDataBuf=0;
    while (DataBuf[pDataBuf]!='\0')
    {
        USART_SendData(USART3,DataBuf[pDataBuf]);
        while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
        pDataBuf++;
        
    }
    
}
/*调试函数*/
void USART3_RE_Deal(void)
{
	if ((USART3_RX_STA&(1<<15)))
	{

		u3_printf("%s\r\n",USART3_RX_BUF);

		USART3_RX_STA=0;
	}
}
