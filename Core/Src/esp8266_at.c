#include "esp8266_at.h"
/*БЛОК ВЫБОРА РЕЖИМА РАБОТЫ ESP8266*/
//USART2 МАССИВ ОТПРАВКА И ПОЛУЧЕНИЯ ДАННЫХ
uint8_t usart2_txbuf[256];
uint8_t usart2_rxbuf[512];
uint8_t usart2_rxone[1];
uint8_t usart2_rxcounter;


//ПОСЛЕДОВАТЕЛЬНЫЙ ПОРТ ОТПРАВЛЯЕТ 1 БАЙТ
static void USART2_SendOneByte(uint8_t val)
{
	((UART_HandleTypeDef *)&huart2)->Instance->DR = ((uint16_t)val & (uint16_t)0x01FF);
	while((((UART_HandleTypeDef *)&huart2)->Instance->SR&0X40)==0);//ЗАВЕРШЕНИЕ ДОСТАВКИ БАЙТА
}


//ОТПРАВКА ДАННЫХ НА ESP8266 ФИКСИРОВАННОЙ ДЛИНЫ
void ESP8266_ATSendBuf(uint8_t* buf,uint16_t len)
{
	memset(usart2_rxbuf,0, 256);

	//УСТАНОВКА ОБЩИХ КОЛИЧЕСТВО ПРИНЯТЫХ ПОСЛЕДОВАТЕЛЬНЫХ ПОРТОВ РАВНЫМ 0 ПЕРЕД КАЖДОЙ ПЕРЕДАЧЕЙ
	usart2_rxcounter = 0;

	//ДОСТАВКА ФИКСИРОВАНОЙ ДЛИНЫ ПО USART2
	HAL_UART_Transmit(&huart2,(uint8_t *)buf,len,0xFFFF);
}

//ОТПРАВКА СТРОКИ В ESP8266
void ESP8266_ATSendString(char* str)
{
  memset(usart2_rxbuf,0, 256);

	//УСТАНОВКА ОБЩИХ КОЛИЧЕСТВО ПРИНЯТЫХ ПОСЛЕДОВАТЕЛЬНЫХ ПОРТОВ РАВНЫМ 0 ПЕРЕД КАЖДОЙ ПЕРЕДАЧЕЙ
	usart2_rxcounter = 0;

	//СПОСОБ ОТПРАВКИ 1
	while(*str)		USART2_SendOneByte(*str++);

	//СПОСОБ ОТПРАВКИ 2
//	HAL_UART_Transmit(&huart1,(uint8_t *)str,strlen(str),0xFFFF);
}

//ВЫХОД ИЗ ПРОЗРАЧНОГО РЕЖИМА РАБОТЫ ESP8266
void ESP8266_ExitUnvarnishedTrans(void)
{
	ESP8266_ATSendString("+++");
	HAL_Delay(50);
	ESP8266_ATSendString("+++");
	HAL_Delay(50);
}

//ПРОВЕРКА СОДЕРЖИТ ЛИ СТРОКА ДРУГУЮ СТРОКУ
uint8_t FindStr(char* dest,char* src,uint16_t retry_nms)
{
	retry_nms/=10;                   //ПЕРЕРЫВ

	while(strstr(dest,src)==0 && retry_nms--)//ОЖИДАНИЕ ПОКА ПОСЛЕДОВАТЕЛЬНЫЙ ПОРТ ПРИМЕТ ИЛИ ЗАВЕРШИТ РАБОТУ ПОСЛЕ ПЕРЕРЫВА
	{
		HAL_Delay(10);
	}

	if(retry_nms) return 1;

	return 0;
}


uint8_t ESP8266_Check(void)
{
	uint8_t check_cnt=5;
	while(check_cnt--)
	{
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf)); 	 //ОЧИСТКА БУФЕРА ПРИЕМА
		ESP8266_ATSendString("AT\r\n");     		 			//ОТПРАВИТЬ ПО КОМАНДУ
		if(FindStr((char*)usart2_rxbuf,"OK",200) != 0)
		{
			return 1;
		}
	}
	return 0;
}

/**
 * ФУНКЦИЯ ИНИЦИАЛИЗАЦИИ ESP8266
 * ВОЗВРАЩАЕМОЕ ЗНАЧЕНИЕ: РЕЗУЛЬТАТ ИНИЦИАЛИЗАЦИЯ ОТЛИЧНЫЙ ОТ 1 - УСПЕШНАЯ, 0 - СБОЙ
 */
uint8_t ESP8266_Init(void)
{
	//ОЧИСТКА МАССИВА ОТПРАВКИ И ПОЛУЧЕНИЯ
	memset(usart2_txbuf,0,sizeof(usart2_txbuf));
	memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));

	ESP8266_ExitUnvarnishedTrans();		//ВЫХОД ИЗ ПРОЗРАЧНОЙ ПЕРЕДАЧИ ESP8266
	HAL_Delay(500);
	ESP8266_ATSendString("AT+RST\r\n");
	HAL_Delay(800);
	if(ESP8266_Check()==0)              //ИСПОЛЬЗОВАНИЕ КОМАНДЫ AT ЧТОБЫ ПРОВЕРИТЬ ЖИВА ЛИ ESP8266
	{
		return 0;
	}

	memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));    //ОЧИСТКА БУФЕРА ПРИЕМА
	ESP8266_ATSendString("ATE0\r\n");     	//ВЫКЛЮЧЕНИЕ ЭХО
	if(FindStr((char*)usart2_rxbuf,"OK",500)==0)  //НЕУДАЧНАЯ НАСТРОЙКА
	{
			return 0;
	}
	return 1;                         //УСПЕШНО НАСТРОЕНА
}

/**
 *ФУНКЦИЯ: ВОССТАНОВЛЕНИЕ ЗАВОДСКИХ НАСТРОЕК
 * В ЭТО ВРЕМЯ ВСЕ ПОЛЬЗОВАТЕЛЬСКИЕ НАСТРОЙКИВ ESP8266 БУДУТ ПОТЕРЯНЫ И
 * ВОССТАНОВЛЕНЫ ДО ЗАВОДСКОГО СОСТОЯНИЯ
 */
void ESP8266_Restore(void)
{
	ESP8266_ExitUnvarnishedTrans();          	//ВЫХОД ИЗ ПРОЗРАЧНОЙ ПЕРЕДАЧИ
  HAL_Delay(500);
	ESP8266_ATSendString("AT+RESTORE\r\n");		//КОМАНДА AT ДЛЯ СБРОСА ДО ЗАВОДСКИХ НАСТРОЙК
}

/**
 * ФУНКЦИЯ ПОДКЛЮЧЕНИЕ К ТОЧКЕ ДОСТУПА
 * ПАРАМЕТРЫ：
 *         ssid:ЛОГИН
 *         pwd:ПАРОЛЬ
 * ВОЗВРАЩАЕМОЕ ЗНАЧЕНИЕ：
 *        РЕЗУЛЬТАТ ПОДКЛЮЧЕНИЕ 1 - СОЕДИНЕНИЕ БЫЛО УСПЕШНЫМ, 0 - СОЕДИНЕНИЕ НЕ УДАЛОСЬ
 *         ПРИЧИНЫ СБОЯ
 *         1. НЕВЕРНОЕ ИМЯ ИЛИ ПАРОЛЬ
 *         2. В МАРШРУТИЗАТОР ПОДКЛЮЧЕНО СЛИШКОМ МНОГО УСТРОЙСТВ И НЕ МОЖЕТ НАЗНАЧИТЬ IP-АДРЕС ESP8266
 */
uint8_t ESP8266_ConnectAP(char* ssid,char* pswd)
{
	uint8_t cnt=5;
	while(cnt--)
	{
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));//ОЧИСТКА БУФЕРА ПРИЕМА
		ESP8266_ATSendString("AT+CWMODE_CUR=1\r\n");              //С ПОМОЩЬЮ КОМАНД AT УСТАНОВИТЬ В РЕЖИМ СТАНЦИИ
		if(FindStr((char*)usart2_rxbuf,"OK",200) != 0)
		{
			break;
		}
	}
	if(cnt == 0)
		return 0;

	cnt=2;
	while(cnt--)
	{
		memset(usart2_txbuf,0,sizeof(usart2_txbuf));//ОЧИСТКА БУФЕРА ОТПРАВКИ
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));//ОЧИСТКА БУФЕРА ПРЕМА
		sprintf((char*)usart2_txbuf,"AT+CWJAP_CUR=\"%s\",\"%s\"\r\n",ssid,pswd);//С ПОМОЩЬЮ КОМАНД AT ПОДКЛЮЧЕНИЕ В ЦЕЛЕВОЙ ТОЧКЕ ДОСТУПА
		ESP8266_ATSendString((char*)usart2_txbuf);
		if(FindStr((char*)usart2_rxbuf,"OK",8000)!=0)                      //СОЕДИНЕНИЕ ВЫПОЛНЕНО УСПЕШНО И ПРИСВОЕН IP-АДРЕСУ
		{
			return 1;
		}
	}
	return 0;
}

//ВКЛЮЧЕНИЕ РЕЖИМА ПРОЗРАЧНОЙ ПЕРЕДАЧИ
static uint8_t ESP8266_OpenTransmission(void)
{
	//УСТАНОВЛЕНИЕ РЕДИМА ПРОЗРАЧНОЙ ПЕРЕДАЧИ
	uint8_t cnt=2;
	while(cnt--)
	{
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));
		ESP8266_ATSendString("AT+CIPMODE=1\r\n");
		if(FindStr((char*)usart2_rxbuf,"OK",200)!=0)
		{
			return 1;
		}
	}
	return 0;
}

/**
 * ФУНКЦИЯ: ИСПОЛЬЗОВАНИЯ УКАЗАННОГО ПРОТОКОЛА (TCP/UDP) ДЛЯ ПОДКЛЮЧЕНИЕ К СЕРВЕРУ
 * ПАРАМЕТРЫ：
 *         РЕЖИМ:ТИП ПРОТОКОЛА "TCP","UDP"
 *         IP: IP ЦЕЛЕВОГО СЕРВЕРА
 *         PORT:НОМЕР ПОРТА СЕРВЕРА
 * ВОЗВРАЩАЕМОЕ ЗНАЧЕНИЕ：
 *         РЕЗУЛЬТАТ ПОДКЛЮЧЕНИЕ, 1 - УСПЕШНО, 0 - НЕ УДАЛОСЬ
 * ОШИБКИ：
 *         ПРИЧИНЫ СБОЯ
 *         1. НЕВЕРНЫЙ IP-АДРЕС И НОМЕР ПОРТА УДАЛЕННОГО СЕРВЕРА
 *         2. НЕ ПОДКЛЮЧЕН К ТОЧКЕ ДОСТУПА
 *         3. ДОБАВЛЕНИЕ НА СТОРОНЕ СЕРВЕРА ЗАПРЕЩЕНО
 */
uint8_t ESP8266_ConnectServer(char* mode,char* ip,uint16_t port)
{
	uint8_t cnt;

	ESP8266_ExitUnvarnishedTrans();                   // ДЛЯ ВЫХОЖА ИЗ ПРОЗРАЧНОЙ СИТУАЦИИ ТРЕБУЕТСЯ НЕСКОЛЬКО ПОДКЛЮЧЕНИЙ
	HAL_Delay(500);

	//ПОДКЛЮЧЕНИЕ К СЕРВЕРУ
	cnt=2;
	while(cnt--)
	{
		memset(usart2_txbuf,0,sizeof(usart2_txbuf));//ОЧИСТКА БУФЕРА ОТПРАВКИ
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));//ОЧИСТКА БУФЕРА ПРИЕМА
		sprintf((char*)usart2_txbuf,"AT+CIPSTART=\"%s\",\"%s\",%d\r\n",mode,ip,port);
		ESP8266_ATSendString((char*)usart2_txbuf);
		if(FindStr((char*)usart2_rxbuf,"CONNECT",8000) !=0 )
		{
			break;
		}
	}
	if(cnt == 0)
		return 0;

	//УСТАНОВЛЕНИЕ ПРОЗРАЧНОГО РЕЖИМА ПЕРЕДАЧИ
	if(ESP8266_OpenTransmission()==0) return 0;

	//ВКЛЮЧЕНИЕ СТАТУСА ОТПРАВКИ
	cnt=2;
	while(cnt--)
	{
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf)); //ОЧИСТКА БУФЕРА ПРИЕМА
		ESP8266_ATSendString("AT+CIPSEND\r\n");//С ПОМОЩЬЮ КОМАНД AT ВКЛЮЧЕНИЕ РЕЖИМА ПРОЗРАЧНОЙ ПЕРЕДАЧИ
		if(FindStr((char*)usart2_rxbuf,">",200)!=0)
		{
			return 1;
		}
	}
	return 0;
}

/**
 * ФУНКЦИЯ：ОТКЛЮЧЕНИЯ ОТ СЕРВЕРА
 * ПАРАМЕТР：НЕТ
 * ВОЗВРАЩАЕМОЕ ЗНАЧЕНИЕ：
 *         ОТКЛЮЧЕНИЕ ОТ СЕРВАРА 1, НЕ УДАЛОСЬ ОТКЛЮЧИТСЯ 0
 */
uint8_t DisconnectServer(void)
{
	uint8_t cnt;

	ESP8266_ExitUnvarnishedTrans();	//ВЫХОД ИЗ ПРОЗРАЧНОЙ ПЕРЕДАЧИ
	HAL_Delay(500);

	while(cnt--)
	{
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf)); // ОЧИСТКА БУФЕРА ПРИЕМА
		ESP8266_ATSendString("AT+CIPCLOSE\r\n");//С ПОМОЩЬЮ КОМАНДЫ AT ЗАКРЫТЬ TCP/UDP СОЕДИНЕНИЕ

		if(FindStr((char*)usart2_rxbuf,"CLOSED",200)!=0)//ОПЕРАЦИЯ ПРОШЛА УСПЕШНО, И СЕРВЕР БЫЛ УСПЕШНО ОТКЛЮЧЕН
		{
			break;
		}
	}
	if(cnt) return 1;
	return 0;
}
