#include "esp8266_mqtt.h"
#include "esp8266_at.h"

//СОЕДИНЕНИЕ ПРОШЛО УСПЕШНО СЕРВЕР ОТВЕТИЛ 20 02 00 00
//КЛИЕНТ АКТИВНО ОТКЛЮЧАЕТСЯ e0 00
const uint8_t parket_connetAck[]   = 	{0x20,0x02,0x00,0x00};
const uint8_t parket_disconnet[]   = 	{0xe0,0x00};
const uint8_t parket_heart[] 	   = 	{0xc0,0x00};
const uint8_t parket_heart_reply[] = 	{0xc0,0x00};
const uint8_t parket_subAck[]      = 	{0x90,0x03};

volatile uint16_t MQTT_TxLen;

//MQTT ОТПРАВЛЯЕТ ДАННЫЙ
void MQTT_SendBuf(uint8_t *buf,uint16_t len)
{
	ESP8266_ATSendBuf(buf,len);
}

//ОТПРАВЛЯЕТ ПАКЕТ ПРОВЕРКИ
void MQTT_SentHeart(void)
{
	MQTT_SendBuf((uint8_t *)parket_heart,sizeof(parket_heart));
}

//MQTT ОТКЛЮЧЕНИЕ
void MQTT_Disconnect()
{
	MQTT_SendBuf((uint8_t *)parket_disconnet,sizeof(parket_disconnet));
}

//MQTT ИНИЦИАЛИЗАЦИЯ
void MQTT_Init(uint8_t *prx,uint16_t rxlen,uint8_t *ptx,uint16_t txlen)
{
	memset(usart2_txbuf,0,sizeof(usart2_txbuf)); //ОЧИСТКА БУФЕРА ОТПРАВКИ
	memset(usart2_rxbuf,0,sizeof(usart2_rxbuf)); //ОЧИСТКА БУФЕРА ПРИЕМА

	//ОТКЛЮЧЕНИЕ
	MQTT_Disconnect();HAL_Delay(100);
	MQTT_Disconnect();HAL_Delay(100);
}

//ФУНКЦИЯ УПАКОВКИ ДЛЯ СЕРВЕРА СОЕДИНЕНИЯ MQTT
uint8_t MQTT_Connect(char *ClientID,char *Username,char *Password)
{
	int ClientIDLen = strlen(ClientID);
	int UsernameLen = strlen(Username);
	int PasswordLen = strlen(Password);
	int DataLen;
	MQTT_TxLen=0;
	//ЗАГОЛОВОК ПЕРЕМЕННОЙ + ПОЛЕЗНАЯ НАГРУЗКА, КАЖДОЕ ПОЛЕ СОДЕРЖИТ ИДЕНТИФИКАТОР ДЛИННОЙ В ДВА БАЙТА
  DataLen = 10 + (ClientIDLen+2) + (UsernameLen+2) + (PasswordLen+2);

	//ФИКСИРОВАНЫ ЗАГОЛОВОК
	//ТИП УПРАВЛЯЮЩЕГО СОЕДИНЕНИЯ
  usart2_txbuf[MQTT_TxLen++] = 0x10;		//ТИП СООБЩЕНИЕ MQTT CONNECT
	//ОСТАВШАЯСЯ ДЛИНА
	do
	{
		uint8_t encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// ЕСЛИ ДЛЯ КОДИРОВАНИЯ ТРЕБУЕТСЯ БОЛЬШЕ ДАННЫХ, УСТАНОВИТЕ ВЕРХНИЙ БИТ ЭТОГО БАЙТА
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		usart2_txbuf[MQTT_TxLen++] = encodedByte;
	}while ( DataLen > 0 );

	//ЗАГОЛОВОК ПЕРЕМЕННОЙ
	//ИМЯ ПРОТОКОЛА
	usart2_txbuf[MQTT_TxLen++] = 0;        		// ДЛИНА ИМЕНИ ПРОТОКОЛА MSB
	usart2_txbuf[MQTT_TxLen++] = 4;        		// ДЛИНА ИМЕНИ ПРОТОКОЛА LSB
	usart2_txbuf[MQTT_TxLen++] = 'M';        	// ASCII КОД ДЛЯ M
	usart2_txbuf[MQTT_TxLen++] = 'Q';        	// ASCII КОД ДЛЯ Q
	usart2_txbuf[MQTT_TxLen++] = 'T';        	// ASCII КОД ДЛЯ T
	usart2_txbuf[MQTT_TxLen++] = 'T';        	// ASCII КОД ДЛЯ T
	//УРОВЕНЬ ПРОТОКОЛА
	usart2_txbuf[MQTT_TxLen++] = 4;        		// MQTT ВЕРСИЯ ПРОТОКОЛА = 4
	//ЗНАК ПОДКЛЮЧЕНИЕ
	usart2_txbuf[MQTT_TxLen++] = 0xc2;        	// ФЛАГ СОЕДИНЕНИЯ
	usart2_txbuf[MQTT_TxLen++] = 0;        		// ПРОДОЛЖИТЕЛЬНОСТЬ ВРЕМЕНИ ПОДДЕРЖАНИЯ РАБОТОСПОСОБНОСТИ MSB
	usart2_txbuf[MQTT_TxLen++] = 60;        	// ПРОДОЛЖИТЕЛЬНОСТЬ ВРЕМЕНИ ПОДДЕРЖАНИЯ РАБОТОСПОСОБНОСТИ LSB

	usart2_txbuf[MQTT_TxLen++] = BYTE1(ClientIDLen);// ДЛИНА ИДЕНТИФИКАТОРА КЛИЕНТА MSB
	usart2_txbuf[MQTT_TxLen++] = BYTE0(ClientIDLen);// ДЛИНА ИДЕНТИФИКАТОРА КЛИЕНТА LSB
	memcpy(&usart2_txbuf[MQTT_TxLen],ClientID,ClientIDLen);
	MQTT_TxLen += ClientIDLen;

	if(UsernameLen > 0)
	{
		usart2_txbuf[MQTT_TxLen++] = BYTE1(UsernameLen);		//ДЛИНА ЛОГИНА MSB
		usart2_txbuf[MQTT_TxLen++] = BYTE0(UsernameLen);    	//ДЛИНА ЛОГИНА LSB
		memcpy(&usart2_txbuf[MQTT_TxLen],Username,UsernameLen);
		MQTT_TxLen += UsernameLen;
	}

	if(PasswordLen > 0)
	{
		usart2_txbuf[MQTT_TxLen++] = BYTE1(PasswordLen);		//ДЛИНА ПАРОЛЯ MSB
		usart2_txbuf[MQTT_TxLen++] = BYTE0(PasswordLen);    	//ДЛИНА ПАРОЛЯ LSB
		memcpy(&usart2_txbuf[MQTT_TxLen],Password,PasswordLen);
		MQTT_TxLen += PasswordLen;
	}

	uint8_t cnt=2;
	uint8_t wait;
	while(cnt--)
	{
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));
		MQTT_SendBuf(usart2_txbuf,MQTT_TxLen);
		wait=30;//ОЖИДАНИЕ 3 СЕКУНДЫ
		while(wait--)
		{
			//CONNECT
			if(usart2_rxbuf[0]==parket_connetAck[0] && usart2_rxbuf[1]==parket_connetAck[1]) //УСПЕШНОЕ ПОДКЛЮЧЕНИЕ
			{
				return 1;//УСПЕШНОЕ ПОДКЛЮЧЕНИЕ
			}
			HAL_Delay(100);
		}
	}
	return 0;
}

//ФУНКЦИЯ: УПАКОВКИ ДАННЫХ ПОДПИСКИ/ОТКАЗА ОТ ПОДПИСКИ MQTT
//topic       ПРЕДМЕТ
//qos         УРОВЕНЬ СООБЩЕНИЯ
//whether     ПАКЕТ ЗАПРОСОВ НА ПОДПИСКУ/ОТКАЗ ОТ ПОДПИСКИ
uint8_t MQTT_SubscribeTopic(char *topic,uint8_t qos,uint8_t whether)
{
	MQTT_TxLen=0;
	int topiclen = strlen(topic);

	int DataLen = 2 + (topiclen+2) + (whether?1:0);//ДЛИНА ЗАГОЛОВКА ПЕРЕМЕННОЙ (2 БАЙТА) + ДЛИНА ПОЛЕЗНОЙ НАГРУЗКИ
	//ФИКСИРОВАННЫЙ ЗАГОЛОВОК
	//ТИП УПРАВЛЯЮЩЕГО СООБЩЕНИЯ
	if(whether) usart2_txbuf[MQTT_TxLen++] = 0x82; //ТИП СООБЩЕНИЕ И ПОДПИСКА НА ЛОГОТИП
	else	usart2_txbuf[MQTT_TxLen++] = 0xA2;    //ОТПИСКА

	//ОСТАВШАЯСЯ ДЛИНА
	do
	{
		uint8_t encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// ЕСЛИ ДЛЯ КОДИРОВАНИЯ ТРЕБУЕТСЯ БОЛЬШЕ ДАННЫХ, УСТАНОВИТЕ ВЕРХНИЙ БИТ ЭТОГО БАЙТА
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		usart2_txbuf[MQTT_TxLen++] = encodedByte;
	}while ( DataLen > 0 );

	//ЗАГОЛОВОК ПЕРЕМЕННОЙ
	usart2_txbuf[MQTT_TxLen++] = 0;				//ИНДИФИКАТОР СООБЩЕНИЕ MSB
	usart2_txbuf[MQTT_TxLen++] = 0x01;           //ИНДИФИКАТОР СООБЩЕНИЕ LSB
	//ПОЛЕЗНЫЕ НАГРУЗКИ
	usart2_txbuf[MQTT_TxLen++] = BYTE1(topiclen);//ДЛИНА ТЕМЫ MSB
	usart2_txbuf[MQTT_TxLen++] = BYTE0(topiclen);//ДЛИНА ТЕМЫ LSB
	memcpy(&usart2_txbuf[MQTT_TxLen],topic,topiclen);
	MQTT_TxLen += topiclen;

	if(whether)
	{
		usart2_txbuf[MQTT_TxLen++] = qos;//УРОВЕНЬ QOS
	}

	uint8_t cnt=2;
	uint8_t wait;
	while(cnt--)
	{
		memset(usart2_rxbuf,0,sizeof(usart2_rxbuf));
		MQTT_SendBuf(usart2_txbuf,MQTT_TxLen);
		wait=30;//ОЖИДАНИЕ 3 СЕКУНДУ
		while(wait--)
		{
			if(usart2_rxbuf[0]==parket_subAck[0] && usart2_rxbuf[1]==parket_subAck[1]) //ПОДПИСКА ПРОШЛА УСПЕШНО
			{
				return 1;
			}
			HAL_Delay(100);
		}
	}
	if(cnt) return 1;
	return 0;
}

//ФУНКЦИЯ УПАКОВКИ ДАННЫХ ПУБЛИКАЦИИ MQTT
//topic   ПРЕДМЕТ
//message НОВОСТЬ
//qos     УРОВЕНЬ СООБЩЕНИЯ
uint8_t MQTT_PublishData(char *topic, char *message, uint8_t qos)
{
	int topicLength = strlen(topic);
	int messageLength = strlen(message);
	static uint16_t id=0;
	int DataLen;
	MQTT_TxLen=0;
	//ДЛИНА ПОЛЕЗНОЙ НАГРУЗКИ РАССЧИТЫВАЕТСЯ СЛЕДУЮЩИМ ОБРАЗОВ: ВЫЧТИТЕ ДЛИНУ ЗАГОЛОВКА ПЕРЕМЕННОЙ ИЗ ЗНАЧЕНИ ЯПОЛЯ ОСТАВШЕЙСЯ ДЛИНЫ В ФИКСИРОВАННО ЗАГОЛОВКЕ
	//НЕТ ИДЕНТИФИКАТОРА, КОГДА QOS РАВЕН 0
	//ДЛИННА ДАННЫХ            ИМЯ СУБЪЕКТА   ИДЕНТИФИКАТОР ПАКЕТА  ПОЛЕЗНАЯ НАГРУЗКА
	if(qos)	DataLen = (2+topicLength) + 2 + messageLength;
	else	DataLen = (2+topicLength) + messageLength;

    //ФИКСИРОВАННЫЙ ЗАГОЛОВОК
	//ТИП УПРАВЛЯЮЩЕГО СООБЩЕНИЯ
	usart2_txbuf[MQTT_TxLen++] = 0x30;    // MQTT ТИП СООБЩЕНИЕ PUBLISH

	//ОСТАВШАЯСЯ ДЛИНА
	do
	{
		uint8_t encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// ЕСЛИ ДЛЯ КОДИРОВАНИЯ ТРЕБУЕТСЯ БОЛЬШЕ ДАННЫХ, УСТАНОВИТЕ ВЕРХНИЙ БИТ ЭТОГО БАЙТА
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		usart2_txbuf[MQTT_TxLen++] = encodedByte;
	}while ( DataLen > 0 );

	usart2_txbuf[MQTT_TxLen++] = BYTE1(topicLength);//ДЛИНА ТЕМЫ MSB
	usart2_txbuf[MQTT_TxLen++] = BYTE0(topicLength);//ДЛИНА ТЕМЫ LSB
	memcpy(&usart2_txbuf[MQTT_TxLen],topic,topicLength);//КОПИРОВАТЬ  ТЕМУ
	MQTT_TxLen += topicLength;

	//ИДЕНТИФИКАТОР СООБЩЕНИЯ
	if(qos)
	{
			usart2_txbuf[MQTT_TxLen++] = BYTE1(id);
			usart2_txbuf[MQTT_TxLen++] = BYTE0(id);
			id++;
	}
	memcpy(&usart2_txbuf[MQTT_TxLen],message,messageLength);
  MQTT_TxLen += messageLength;

	MQTT_SendBuf(usart2_txbuf,MQTT_TxLen);
  return MQTT_TxLen;
}
