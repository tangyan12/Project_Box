#include "stm32f10x.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "MyDelay.h"

void MY_UART1_Init(uint32_t baud)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);	
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate=baud;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode=USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity=USART_Parity_No;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART1,&USART_InitStructure);
	
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	USART_ClearFlag(USART1,USART_FLAG_RXNE);
	USART_ITConfig(USART1,USART_IT_IDLE,ENABLE);
	USART_ClearFlag(USART1,USART_FLAG_IDLE);
	

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel=USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&NVIC_InitStructure);
	
    
	USART_Cmd(USART1,ENABLE);

}

#define USART1_RX_MAX_SIZE 40
uint16_t USART1_RX_STA;
uint8_t USART1_RX_BUF[USART1_RX_MAX_SIZE];

void USART1_IRQHandler(void)
{
	
    if (USART_GetITStatus(USART1,USART_IT_RXNE)==SET)
    {
        if (!(USART1_RX_STA & (1<<15)))
		{
			USART1_RX_BUF[(USART1_RX_STA&0x7FFF)]=USART_ReceiveData(USART1);
			USART1_RX_STA++;
			USART1_RX_BUF[USART1_RX_STA&0x7FFF]='\0';
			if ((USART1_RX_STA&0x7FFF)>=USART1_RX_MAX_SIZE-1)
			{
				USART1_RX_STA |= (1<<15);				
				
			}	
		}
    	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
	if (USART_GetITStatus(USART1,USART_IT_IDLE)==SET)
	{
		USART1_RX_STA |= (1<<15);
		USART_ReceiveData(USART1);
		USART_ClearFlag(USART1,USART_FLAG_IDLE);
	}
	
	
}
#define DATA_MAX_SIZE 100

void u1_printf(char *format,...)
{
	
	char DataBuf[DATA_MAX_SIZE];
    va_list start;
    va_start(start,format);
    vsprintf(DataBuf,format,start);
    va_end(start);

    uint8_t pDataBuf=0;
    while (DataBuf[pDataBuf]!='\0')
    {
        USART_SendData(USART1,DataBuf[pDataBuf]);
        while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);
        pDataBuf++;
        
    }
    
}
/*调试函数*/
void USART1_RE_Deal(void)
{
	if ((USART1_RX_STA&(1<<15)))
	{
		/*OLED调试和串口调试*/
		// u1_printf("%s\r\n",USART1_RX_BUF);

		USART1_RX_STA=0;
	}
}

char *USART1_check_cmd(char * str)
{
	char *str_return=NULL;
	if (USART1_RX_STA&(1<<15))
	{
		USART1_RX_BUF[USART1_RX_STA&0x7FFF]='\0';
		str_return=strstr(USART1_RX_BUF,str);
		USART1_RX_STA=0;
	}
	return str_return;
}

uint8_t USART1_send_cmd(char *cmd,char *call,uint8_t wait_time)
{
	u1_printf("%s\r\n",cmd);
	if (call&&wait_time)
	{
		while (wait_time--)
		{
//			Delay_ms(10);
			if (USART1_check_cmd(call)!=NULL)
			{
				return 1;
			}
		}
	}
	return 0;
}

void USART1_WIFI_START_Trans(void)
{
	
	USART1_send_cmd("AT+CWMODE=2","OK",50);
	USART1_send_cmd("AT+RST","ready",20);
//	Delay_ms(1000);
//	Delay_ms(1000);
//	Delay_ms(1000);
	USART1_send_cmd("AT+CWSAP=\"ABC\",\"12345678\",11,2","OK",100);
	USART1_send_cmd("AT+CIPMUX=1","OK",20);
	USART1_send_cmd("AT+CIPSERVER=1,8080","OK",50);
	
	USART1_RX_STA=0;
}
#define PWSD "20210909"

void USART1_check_pwsd(void)
{
	if (USART1_RX_STA&(1<<15))
	{
		char * temp;
		temp = strstr(USART1_RX_BUF,PWSD);
		if (temp)
		{
			if (strcmp(temp,PWSD)==0)
			{
			}
			else
			{
			}
		}
		USART1_RX_STA=0;
	}
	
}
