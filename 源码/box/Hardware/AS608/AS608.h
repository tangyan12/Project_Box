#ifndef __MY_AS608__H__
#define __MY_AS608__H__
void AS608_GPIO_Init(void);
void MY_UART2_Init(uint32_t baud);
void u2_printf(char *format,...);
void USART2_RE_Deal(void);
uint8_t AS608_Search(u16 start_page,u16 search_page_circle);
uint8_t AS608_Login(uint16_t storeID);
uint8_t AS608_PS_DeleteChar(uint16_t storeID);
#endif

