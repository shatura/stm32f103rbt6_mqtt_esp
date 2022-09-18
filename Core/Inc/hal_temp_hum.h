#ifndef _HAL_HEMP_HUM_H
#define _HAL_HEMP_HUM_H

#include <stdio.h>
#include "stm32f1xx_hal.h"

#include "main.h"
#include "tim.h"

typedef unsigned          char uint8_t;
typedef unsigned short     int uint16_t;

//Установите порт ввода-вывода драйвера
#define DHT11_DQ_OUT_1 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)
#define DHT11_DQ_OUT_0 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET)
#define DHT11_DQ_IN HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)

#define MEAN_NUM            10

typedef struct
{
    uint8_t curI;
    uint8_t thAmount;
    uint8_t thBufs[10][2];
}thTypedef_t;

/* Function declaration */
uint8_t dht11Init(void); //Инициализация DHT11
uint8_t dht11Read(uint8_t *temperature, uint8_t *humidity); //Чтение DHT11 значения
void dht11SensorTest(void);

#endif /*_HAL_HEMP_HUM_H*/
