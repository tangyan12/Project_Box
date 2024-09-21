#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "MyUART3.h"

#define Delay_ms(x) vTaskDelay(pdMS_TO_TICKS(x))

#define USART_X           USART2
#define USART_X_CLOCK     RCC_APB1Periph_USART2 
#define USART_X_PORT	  GPIOA
#define USART_X_PORT_CLK  RCC_APB2Periph_GPIOA
#define USART_X_RXD       GPIO_Pin_3
#define USART_X_TXD       GPIO_Pin_2
/*中断使能由于又要开启中断又要清除中断，USART_IT和USART_FLAG两个参数是不同的，故不用宏定*/
#define USART_X_IRQ 	  USART2_IRQn
#define USART_X_IT_PRIORITY 3
 /*
*@brief AS608引脚的使能，当有手指放在传感器上，会输入高电*
*@param 
*@retval 
*/
#define AS_GPIO_PORT GPIOA 
#define AS_GPIO_PIN	GPIO_Pin_7
void AS608_GPIO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin=AS_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(AS_GPIO_PORT,&GPIO_InitStructure);
}

void MY_UART2_Init(uint32_t baud)
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

/*这下面的请用Source insight全部替代*/

#define USART2_RX_MAX_SIZE 40
uint16_t USART2_RX_STA;
uint8_t USART2_RX_BUF[USART2_RX_MAX_SIZE];

void USART2_IRQHandler(void)
{
	
    if (USART_GetITStatus(USART2,USART_IT_RXNE)==SET)
    {
        if (!(USART2_RX_STA & (1<<15)))
		{
			USART2_RX_BUF[(USART2_RX_STA&0x7FFF)]=USART_ReceiveData(USART2);
			USART2_RX_STA++;
			USART2_RX_BUF[USART2_RX_STA&0x7FFF]='\0';
			if ((USART2_RX_STA&0x7FFF)>=USART2_RX_MAX_SIZE-1)
			{
				USART2_RX_STA |= (1<<15);				
				
			}	
		}
    	USART_ClearITPendingBit(USART2,USART_IT_RXNE);
    }
	if (USART_GetITStatus(USART2,USART_IT_IDLE)==SET)
	{
		USART2_RX_STA |= (1<<15);
		USART_ReceiveData(USART2);
		USART_ClearFlag(USART2,USART_FLAG_IDLE);
	}
	
	
}
#define DATA_MAX_SIZE 100

void u2_printf(char *format,...)
{
	
	char DataBuf[DATA_MAX_SIZE];
    va_list start;
    va_start(start,format);
    vsprintf(DataBuf,format,start);
    va_end(start);

    uint8_t pDataBuf=0;
    while (DataBuf[pDataBuf]!='\0')
    {
        USART_SendData(USART2,DataBuf[pDataBuf]);
        while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
        pDataBuf++;
        
    }
    
}
/*调试函数*/
void USART2_RE_Deal(void)
{
	if ((USART2_RX_STA&(1<<15)))
	{

		u2_printf("%s\r\n",USART2_RX_BUF);

		USART2_RX_STA=0;
	}
}
/*以下是AS608的各种命*/

uint32_t AS608_Addr = 0xFFFFFFFF;
uint16_t AS608_Head = 0xEF01;
uint8_t AS608_Cmd_Flag = 0x01; 
uint8_t AS608_Callback_Flag = 0x07;


 /*
*@brief 各种基本的发送数据函数，组成命令
*@param 
*@retval 
*/
void AS608_Send_Data(uint8_t Data)
{
	USART_SendData(USART2,Data);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
}
void AS608_Send_Head(void)
{
	AS608_Send_Data(AS608_Head>>8);
	AS608_Send_Data(AS608_Head);
}

void AS608_Send_Addr(void)
{
	AS608_Send_Data(AS608_Addr>>24);
	AS608_Send_Data(AS608_Addr>>16);
	AS608_Send_Data(AS608_Addr>>8);
	AS608_Send_Data(AS608_Addr);
}

void AS608_Send_Flag(uint8_t Flag)
{
	AS608_Send_Data(Flag);
}

void AS608_Send_Length(uint16_t Length)
{
	AS608_Send_Data(Length>>8);
	AS608_Send_Data(Length);
}

void AS608_Send_Sum(uint16_t sum)
{
	AS608_Send_Data(sum>>8);
	AS608_Send_Data(sum);
}



/*这是两个检查应答包确认码的函数*/
uint8_t AS608_Analyse(uint8_t return_code)
{
	if (return_code==0x00)
	{
		u3_printf("返回成功\r\n");
		return 0;
	}
	else if (return_code==0x01)
	{
		u3_printf("返回失败,错误代码1\r\n");
		return 1;
	}
	else if (return_code==0x02)
	{
		u3_printf("返回失败,错误代码2\r\n");
		return 2;
	}
	else if (return_code==0x03)
	{
		u3_printf("返回失败,错误代码3\r\n");
		return 3;
	}
	else if (return_code==0x10)
	{
		u3_printf("删除模板失败\r\n");
		return 10;
	}
	
	else 
	{
		u3_printf("返回失败,错误代码4\r\n");
		return 4;
	}
}
char *AS608_Check_Message(void)
{	
	uint8_t str[8]={0};
	str[0]=AS608_Head>>8;str[1]=AS608_Head;
	str[2]=AS608_Addr>>24;str[3]=AS608_Addr>>16;
	str[4]=AS608_Addr>>8;str[5]=AS608_Addr;
	str[6]=AS608_Callback_Flag;str[7]='\0';

	char * str_search=NULL;

	if (USART2_RX_STA&(1<<15))
	{
		str_search = strstr(USART2_RX_BUF,str);
		USART2_RX_STA=0;
		if (str_search!=NULL)
		{
			str_search=&str_search[9];
		}
	}
	return str_search;
}




uint8_t AS608_PS_GetImage(void)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0003);
	AS608_Send_Data(0x01);
	sum=AS608_Cmd_Flag+0x0003+0x01;
	AS608_Send_Sum(sum); 

	uint8_t result=1;
	unsigned char *str_search=0x00;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result = AS608_Analyse(*str_search);
	}


	return result;
}
uint8_t AS608_PS_GenChar_1(void)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0004);
	AS608_Send_Data(0x02);/*指令*/
	AS608_Send_Data(0x01);/*bufferID*/
	sum=AS608_Cmd_Flag+0x0004+0x02+0x01;
	AS608_Send_Sum(sum); 

	uint8_t result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result = AS608_Analyse(*str_search);
	}


	return result;
}
uint8_t AS608_PS_GenChar_2(void)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0004);
	AS608_Send_Data(0x02);/*指令*/
	AS608_Send_Data(0x02);/*bufferID*/
	sum=AS608_Cmd_Flag+0x0004+0x02+0x02;
	AS608_Send_Sum(sum); 

	uint8_t result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result = AS608_Analyse(*str_search);
	}


	return result;
}
uint8_t AS608_PS_RegModel(void)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0003);
	AS608_Send_Data(0x05);/*指令*/
	sum=AS608_Cmd_Flag+0x0003+0x05;
	AS608_Send_Sum(sum); 

	uint8_t result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result = AS608_Analyse(*str_search);
	}


	return result;
}
uint8_t AS608_PS_StoreChar(uint16_t storeID)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0006);
	AS608_Send_Data(0x06);/*指令*/
	AS608_Send_Data(0x02);/*bufferID*/
	AS608_Send_Data(storeID>>8);/*存入指纹ID的前八位*/
	AS608_Send_Data(storeID);/*存入指纹ID的后八位*/
	sum=AS608_Cmd_Flag+0x0006+0x06+0x02+ \
	(u8)(storeID>>8)+(u8)(storeID);
	AS608_Send_Sum(sum); 

	uint8_t result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result = AS608_Analyse(*str_search);
	}


	return result;
}

uint8_t AS608_PS_DeleteChar(uint16_t storeID)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0007);
	AS608_Send_Data(0x0C);/*指令*/
	AS608_Send_Data(storeID>>8);/*存入指纹ID的前八位*/
	AS608_Send_Data(storeID);/*存入指纹ID的后八位*/
	AS608_Send_Data(0x00);//这句和下面这句是删除一个指纹数，也就是只删除ID号那*
	AS608_Send_Data(0x01); 
	sum=AS608_Cmd_Flag+0x0007+0x0C+0x01+ \
	(u8)(storeID>>8)+(u8)(storeID);
	AS608_Send_Sum(sum); 

	uint8_t result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result = AS608_Analyse(*str_search);
	}
	return result;
}

uint8_t AS608_PS_Empty(void)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0003);
	AS608_Send_Data(0x0D);/*指令*/
	sum=AS608_Cmd_Flag+0x0003+0x0D;
	AS608_Send_Sum(sum); 

	uint8_t result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result = AS608_Analyse(*str_search);
	}
	return result;
}
/*下面这个命令返回数据有点特殊，会返回匹配到的指纹页数和指纹匹配程度，而且我又想接收确认码*/
typedef struct 
{
	uint16_t pagenum;
	uint16_t score;
	uint8_t result;
}AS608_search_result_t;

AS608_search_result_t AS608_PS_Search(uint16_t start_page,uint16_t search_page_count)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0008);
	AS608_Send_Data(0x04);
	AS608_Send_Data(0x01);
	AS608_Send_Data(start_page>>8);/*起始搜索页前八位*/
	AS608_Send_Data(start_page);/*起始搜索页后八位*/
	AS608_Send_Data(search_page_count>>8);/*要搜索的范围前八*/
	AS608_Send_Data(search_page_count);/*要搜索的范围后八*/
	sum=AS608_Cmd_Flag+0x0008+0x04+0x01+ \
	(u8)(start_page>>8)+(u8)(start_page)+ \
	(u8)(search_page_count>>8)+(u8)(search_page_count);
	AS608_Send_Sum(sum); 

	AS608_search_result_t result;
	result.result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result.result = AS608_Analyse(*str_search);
		result.pagenum=str_search[2]+(str_search[1]<<8);
		result.score=str_search[4]+(str_search[3]<<8);
	}

	return result;
}

AS608_search_result_t AS608_PS_HighSearch(uint16_t start_page,uint16_t search_page_count)
{ 
	uint16_t sum;
	AS608_Send_Head();
	AS608_Send_Addr();
	AS608_Send_Flag(AS608_Cmd_Flag);
	AS608_Send_Length(0x0008);
	AS608_Send_Data(0x1B);
	AS608_Send_Data(0x01);
	AS608_Send_Data(start_page>>8);
	AS608_Send_Data(start_page);
	AS608_Send_Data(search_page_count>>8);
	AS608_Send_Data(search_page_count);
	sum=AS608_Cmd_Flag+0x0008+0x1B+0x01+ \
	(u8)(start_page>>8)+(u8)(start_page)+ \
	(u8)(search_page_count>>8)+(u8)(search_page_count);
	AS608_Send_Sum(sum); 

	AS608_search_result_t result;
	result.result=1;
	unsigned char *str_search=NULL;
	str_search = AS608_Check_Message();
	if (str_search!=NULL)
	{
		result.result = AS608_Analyse(*str_search);
		result.pagenum=str_search[2]+(str_search[1]<<8);
		result.score=str_search[4]+(str_search[3]<<8);
	}

	return result;
}

uint8_t AS608_Search(u16 start_page,u16 search_page_circle)
{ 
	uint8_t status=1;
	AS608_search_result_t score;
	status=AS608_PS_GetImage();
	if (status==0)
	{
		u3_printf("录入指纹成功，即将开始搜索\r\n");
		Delay_ms(1000);
		status=1;
		status=AS608_PS_GenChar_1();
		if (status==0)
		{
			u3_printf("生成指纹特征成功\r\n");
			Delay_ms(500);
			score=AS608_PS_Search(start_page,search_page_circle);
			if (score.result==0)
			{
				u3_printf("搜索到指纹,id%d,得分%d\r\n",score.pagenum,score.score);
				return 1;
			}
			else
			{
				u3_printf("未搜索到指纹\r\n");
			}
		}
	}
	return 0;
}
uint8_t AS608_Login(uint16_t storeID)
{
	uint8_t status;
	status=AS608_PS_GetImage();
	if (status==0)
	{
		u3_printf("第一次指纹获取成功\r\n");
		Delay_ms(1000);
		status=AS608_PS_GenChar_1();
		if (status==0)
		{
			u3_printf("第一次生成指纹特征成功\r\n");
			Delay_ms(500);
			status=AS608_PS_GetImage();
			if (status==0)
			{
				u3_printf("第二次指纹获取成功\r\n");
				Delay_ms(1000);
				status=AS608_PS_GenChar_2();
				if (status==0)
				{
					u3_printf("第二次生成指纹特征成功\r\n");
					Delay_ms(500);
					status=AS608_PS_RegModel();
					if (status==0)
					{
						u3_printf("指纹合并成功 \r\n");
						Delay_ms(500);
						status = AS608_PS_StoreChar(storeID);
						if (status==0)
						{
							u3_printf("指纹录入成功,待检测\r\n");
							Delay_ms(500);
							status=AS608_Search(0x01,0x64);
							if (status==1)
							{
								u3_printf("检测指纹成功\r\n");
								return 1;
							}
							
						}
						
					}
					
				}
				
			}
			
		}
		
	}
	return 0;	
}
