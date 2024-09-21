#ifndef __MY_UART3__H__
#define __MY_UART3__H__

void MY_UART3_Init(uint32_t baud);
void u3_printf(char *format,...);
void USART3_RE_Deal(void);

#endif
