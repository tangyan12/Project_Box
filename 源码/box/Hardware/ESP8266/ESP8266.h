#ifndef __MY_ESP8266__H__
#define __MY_ESP8266__H__

void MY_UART1_Init(uint32_t baud);
void u1_printf(char *format,...);
void USART1_WIFI_START_Trans(void);
void USART1_RE_Deal(void);
void USART1_check_pwsd(void);
#endif

