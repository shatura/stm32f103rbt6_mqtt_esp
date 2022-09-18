#ifndef __ES8266_MQTT_H
#define __ES8266_MQTT_H

#include "stm32f1xx_hal.h"

#define BYTE0(dwTemp)       (*( char *)(&dwTemp))
#define BYTE1(dwTemp)       (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)       (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)       (*((char *)(&dwTemp) + 3))

//MQTT ПОДКЛЮЧЕНИЕ К СЕРВЕРУ
extern uint8_t MQTT_Connect(char *ClientID,char *Username,char *Password);
//MQTT ПОДПИСКА НА СООБЩЕНИЕ
extern uint8_t MQTT_SubscribeTopic(char *topic,uint8_t qos,uint8_t whether);
//MQTT ПУБЛИКАЦИЯ ДАННЫЙ
extern uint8_t MQTT_PublishData(char *topic, char *message, uint8_t qos);
//MQTT ОТПРАВКА ПАКЕТА НА ОТКЛИК СЕРВЕРА
extern void MQTT_SentHeart(void);

#endif
