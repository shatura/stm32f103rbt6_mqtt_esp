#ifndef __ESP8266_AT_H
#define __ESP8266_AT_H

#include "stm32f1xx_hal.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

extern uint8_t usart2_txbuf[256];
extern uint8_t usart2_rxbuf[512];
extern uint8_t usart2_rxone[1];
extern uint8_t usart2_rxcounter;

extern uint8_t ESP8266_Init(void);
extern void ESP8266_Restore(void);

extern void ESP8266_ATSendBuf(uint8_t* buf,uint16_t len);		//ОТПРАВЛЕНИЕ ДАННЫХ УКАЗАНОЙ ДЛИННЫ НА ESP8266
extern void ESP8266_ATSendString(char* str);								//ОТПРАВЛЕНИЕ СТРОКИ НА ESP8266
extern void ESP8266_ExitUnvarnishedTrans(void);							//ВКЛЮЧЕНИЕ ESP8266 В РЕЖИМ "UART-Wi-Fi passthrough"
extern uint8_t ESP8266_ConnectAP(char* ssid,char* pswd);		//ТОЧКА ДОСТУПА ДЛЯ ПОДКЛЮЧЕНИЕ ESP8266
extern uint8_t ESP8266_ConnectServer(char* mode,char* ip,uint16_t port);	//ПОДКЛЮЧЕНИЕ К СЕРВЕРУ С ИСПОЛЬЗОВАНИЕМ УКАЗАННОГО ПРОТОКОЛА (TCP/UDP)

#endif
