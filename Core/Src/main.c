/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hal_temp_hum.h"
#include "esp8266_at.h"
#include "esp8266_mqtt.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define USER_MAIN_DEBUG

#ifdef USER_MAIN_DEBUG
#define user_main_printf(format, ...) printf( format "\r\n",##__VA_ARGS__)
#define user_main_info(format, ...) printf("【main】info:" format "\r\n",##__VA_ARGS__)
#define user_main_debug(format, ...) printf("【main】debug:" format "\r\n",##__VA_ARGS__)
#define user_main_error(format, ...) printf("【main】error:" format "\r\n",##__VA_ARGS__)
#else
#define user_main_printf(format, ...)
#define user_main_info(format, ...)
#define user_main_debug(format, ...)
#define user_main_error(format, ...)
#endif

//НАСТРОЙКИ В СООТВЕТСТВИИ С ВАШИМ WIFI
#define WIFI_NAME "HappyOneDay"
#define WIFI_PASSWD "1234567890"

//КОНИФГУРАЦИЯ ВХОДА В ОБЛАЧНЫЙ СЕРВЕР
#define MQTT_BROKERADDRESS "a1lAoazdH1w.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define MQTT_CLIENTID "00001|securemode=3,signmethod=hmacsha1|"
#define MQTT_USARNAME "BZL01&a1lAoazdH1w"
#define MQTT_PASSWD "51A5BB10306E976D6C980F73037F2D9496D2813A"
#define	MQTT_PUBLISH_TOPIC "/sys/a1lAoazdH1w/BZL01/thing/event/property/post"
#define MQTT_SUBSCRIBE_TOPIC "/sys/a1lAoazdH1w/BZL01/thing/service/property/set"

//ОПРЕДЕЛЕНИЕ МАКРОСА ЗАДЕРЖКИ ДЛЯ ОПЕРАЦИИ ОСНОВНОГО ЦИКЛА
#define LOOPTIME 30 	//ПРОГРАММНЫЙ ЦИКЛ ВРЕМЯ ЗАДЕРЖКИ：30ms
#define COUNTER_LEDBLINK			(300/LOOPTIME)		//ВРЕМЯ РАБОТЫ СВЕТОДИОДА, ВРЕМЯ МИГАНИЯ：300ms
#define COUNTER_RUNINFOSEND		(5000/LOOPTIME)		//ЗАПРОС НА ЗАПУСК ПОСЛЕДОВАТЕЛЬНОГО ПОРТА：5s
#define COUNTER_MQTTHEART     (5000/LOOPTIME)		//MQTT ОТПРАВКА ПАКЕТА ПРОВЕРКИ：5s
#define COUNTER_STATUSREPORT	(3000/LOOPTIME)		//ЗАГРУЗКА СТАТУСА：3s
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char mqtt_message[300];	//КЭШ СООБЩЕНИЙ ОТЧЕТА MQTT
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Enter_ErrorMode(uint8_t mode);
void ES8266_MQTT_Init(void);
void Change_LED_Status(void);
void STM32DHT11_StatusReport(void);
void deal_MQTT_message(uint8_t* buf,uint16_t len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart2,usart2_rxone,1);			//ОТКРОЙТЕ ПРЕРЫВАНИЙ USART1 И ПОЛУЧИТЕ СООБЩЕНИЕ О ПОДПИСКЕ
	ES8266_MQTT_Init();										//ИНИЦИАЛИЗИРОВАТЬ MQTT
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	uint16_t Counter_RUNInfo_Send = 0;
	uint16_t Counter_StatusReport = 0;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  //ПЕЧАТЬ ТЕКУЩЕГО СОСТОЯНИЯ
	  		if(Counter_RUNInfo_Send++>COUNTER_RUNINFOSEND)
	  		{
	  			Counter_RUNInfo_Send = 0;
	  			user_main_info("ПРОГРАММА ЗАПУЩЕНА！\r\n");
	  		}
	  		//ОТЧЕТ О МЕСТНОМ СТАТУСЕ
	  			if(Counter_StatusReport++>COUNTER_STATUSREPORT)
	  			{
	  				Counter_StatusReport = 0;
	  				STM32DHT11_StatusReport();
	  			}

	  			//ЕСЛИ В ПРИНИМАЮЩЕМ КЭШЕ ЕСТЬ ДАННЫЕ
	  			if(usart2_rxcounter)
	  			{
	  				deal_MQTT_message(usart2_rxbuf,usart2_rxcounter);
	  			}

	  			HAL_Delay(LOOPTIME);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/******************************  USART2 ПОЛУЧАЕТ КОД ПРЕРЫВАНИЯ  *****************************/

// ПОСЛЕДОВАТЕЛЬНЫЙ ПОРТ ДРАЙВЕРА ESP8266 ПОЛУЧАЕТ ФУНКЦИЮ ОБРАБОТКИ ПРЕРЫВАНИЙ
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2)	// ОПРЕДЕЛЕНИЕ, КАКОЙ ПОСЛЕДОВАТЕЛЬНЫЙ ПОРТ ВЫЗВАЛ ПРЕРЫВАНИЕ
	{
		//ПОМЕСТИТЬ ПОЛУЧЕННЫЕ ДАННЫЕ В ПРИНИМАЮЩИЙ МАССИВ USART1 RECEIVING
		usart2_rxbuf[usart2_rxcounter] = usart2_rxone[0];
		usart2_rxcounter++;	//ПОЛУЧЕННОЕ КОЛИЧЕСТВО＋1

		//ПОВТОРНОЕ ВКЛЮЧЕНИЕ ПОСЛЕДОВАТЕЛЬНОГО ПОРТА 1 ДЛЯ ПРИЕМА ПРЕРЫВАНИЙ
		HAL_UART_Receive_IT(&huart2,usart2_rxone,1);
	}
}

/******************************  КЛЮЧЕВОЙ КОД ПРЕРЫВАНИЙ  *****************************/

//КЛАВИША 1, НАЖМИТЕ, ЧТОБЫ ВЫПОЛНИТЬ ФУНКЦИЮ
void KEY1_Pressed(void)
{
	user_main_debug("НАЖАТИЕ KEY_1\r\n");
	Change_LED_Status();
}

//КЛАВИША 2, НАЖМИТЕ, ЧТОБЫ ВЫПОЛНИТЬ ФУНКЦИЮ
void KEY2_Pressed(void)
{
	user_main_debug("НАЖАТИЕ KEY_2\r\n");
}

//КЛЮЧЕВАЯ ФУНКЦИЯ ОБРАБОТКИ ПРЕРЫВАНИЙ
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch(GPIO_Pin)
    {
        case KEY_1_Pin:KEY1_Pressed();break;
        case KEY_2_Pin:KEY2_Pressed();break;
        default:break;
    }
}

//ИЗМЕНЕНИЯ СОСТОЯНИЙ СВЕТОДИОДНОЙ ПОДСВЕТКИ
void Change_LED_Status(void)
{
	static uint8_t ledstatus = 0;

	switch(ledstatus++)
	{
		case 0://000
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET);
		break;
		case 1://001
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		break;
		case 2://010
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET);
		break;
		case 3://011
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		break;
		case 4://100
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET);
		break;
		case 5://101
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		break;
		case 6://110
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET);
		break;
		case 7://111
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		break;
		default:ledstatus=0;break;
	}
}

/******************************  КОД РЕЖИМА ОШИБКИ  *****************************/

//ВОЙДИТЕ В РЕЖИМ ОШИБКИ И ДОЖДИТЕСЬ РУЧНОГО ПЕРЕЗАПУЧКА
void Enter_ErrorMode(uint8_t mode)
{
	HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
	while(1)
	{
		switch(mode){
			case 0:user_main_error("ESP8266 СБОЙ ИНИЦИАЛИЗАЦИИ！\r\n");break;
			case 1:user_main_error("ESP8266 НЕ УДАЛОСЬ ПОДКЛЮЧИТСЯ К ТОЧКЕ ДОСТУПА！\r\n");break;
			case 2:user_main_error("ESP8266 НЕ УДАЛОСЬ ПОДКЛЮЧИТСЯ К ОБЛАЧНОМУ СЕРВЕРУ！\r\n");break;
			case 3:user_main_error("ESP8266 ОШИБКА ВХОДА В СИСТЕМУ MQTT！\r\n");break;
			case 4:user_main_error("ESP8266ОШИБКА В ТЕМЕ ПОДПИСКЕ MQTT！\r\n");break;
			default:user_main_info("НЕЧЕГО\r\n");break;
		}
		user_main_info("ПОЖАЛУЙСТА, ПЕРЕЗАПУСТИТЕ");
		//HAL_GPIO_TogglePin(LED_R_GPIO_Port,LED_R_Pin);
		HAL_Delay(200);
	}
}

/******************************  ТЕСТОВЫЙ КОД ПЕРЕФЕРИЙНЫХ УСТРОЙСТВ ПЛАТЫ РАЗРАБОТКИ  *****************************/

/* ФУНКЦИЯ ТЕСТИРОВАНИЕ ПОСЛЕДОВАТЕЛЬНОГО ПОРТА ОТЛАДКИ */
void TEST_usart1(void)
{
	user_main_debug("ТЕСТОВЫЙ КОД USART1！\n");
	HAL_Delay(1000);
}

/* ФУНКЦИЯ ТЕСТИРОВАНИЯ СВЕТОДИОДА */
void TEST_LED(void)
{
	//КРАСНЫЙ ИНДИКАТОР ГОРИТ ДРУГИЕ ВЫКЛЮЧЕНЫ
	HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
	HAL_Delay(500);
	//ЗЕЛЕНЫЙ ИНДИКАТОР ГОРИТ ДРУГИЕ ВЫКЛЮЧЕНЫ
	HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
	HAL_Delay(500);
	//СИНИЙ ИНДИКАТОР ГОРИТ ДРУГИЕ ВЫКЛЮЧЕНЫ
	HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET);
	HAL_Delay(500);
}

/* ФУНКЦИЯ ПРОВЕРКИ ЗАДЕРЖКИ */
void TEST_delayus(void)
{
	static uint16_t tim1_test;

	//ЗАДЕРЖКА 1 МС
	for(tim1_test = 0;tim1_test<1000;tim1_test++)
	{
		//ЗАДЕРЖКА 1 МС
		TIM1_Delay_us(1000);
	}
	user_main_debug("ТЕСТОВЫЙ КОД ЗАДЕРЖКИ В 1 СЕКУНДУ!\n");
}

/* ФУНКЦИЯ ТЕСТИРОВАНИЕ ДАТЧИКА ТЕМПЕРАТУРЫ И ВЛАЖНОСТИ DHT11 */
void TEST_DHT11(void)
{
	uint8_t temperature;
	uint8_t humidity;
	uint8_t get_times;

	// ПОЛУЧЕНИЕ ИНФОРМАЦИИ О ТЕМПЕРАТУРЕ И ВЛАЖНОСТИ И РАСПЕЧАТАТЬ ЕЕ С ПОМОЩЬЮ ПОСЛЕДОВАТЕЛЬНОГО ПОРТА, ПОЛУЧИТЬ ЕЕ ДЕСЯТЬ РАЗ,ПОКА ОНА УСПЕШНО НЕ ВЫСКОЧИТ
	for(get_times=0;get_times<10;get_times++)
	{
		if(!dht11Read(&temperature, &humidity))//Read DHT11 Value
		{
				user_main_info("temperature=%d,humidity=%d \n",temperature,humidity);
				break;
		}
	}
}

/* ТЕСТОВЫЙ КОД ESP8266 И MQTT */
void TEST_ES8266MQTT(void)
{
	uint8_t status=0;

	//ИНИЦИАЛИЗИРОВАНИЕ
	if(ESP8266_Init())
	{
		user_main_info("ИНИЦИАЛИЗАЦИЯ ESP8266 ПРОШЛА УСПЕШНО！\r\n");
		status++;
	}
	else Enter_ErrorMode(0);

	//ПОДКЛЮЧЕНИЕ К ТОЧКАМ ДОСТУПАМ
	if(status==1)
	{
		if(ESP8266_ConnectAP(WIFI_NAME,WIFI_PASSWD))
		{
			user_main_info("ESP8266 УСПЕШНО ПОДКЛЮЧИЛСЯ К ТОЧКЕ ДОСТУПА！\r\n");
			status++;
		}
		else Enter_ErrorMode(1);
	}

	//ПОДЛКЮЧЕНИЕ К ОБЛАЧНОМУ СЕРВЕРУ
	if(status==2)
	{
		if(ESP8266_ConnectServer("TCP",MQTT_BROKERADDRESS,1883)!=0)
		{
			user_main_info("ESP8266УСПЕШНО ПОДКЛЮЧИЛСЯ К ОБЛАЧНОМУ СЕРВЕРУ！\r\n");
			status++;
		}
		else Enter_ErrorMode(2);
	}

	//ВХОД В MQTT
	if(status==3)
	{
		if(MQTT_Connect(MQTT_CLIENTID, MQTT_USARNAME, MQTT_PASSWD) != 0)
		{
			user_main_info("ESP8266 ОБЛАЧНЫЙ СЕРВЕР MQTT УСПЕШНО ВОШЕЛ В СИСТЕМУ！\r\n");
			status++;
		}
		else Enter_ErrorMode(3);
	}

	//ПОДПИСКА НА ТЕМЫ
	if(status==4)
	{
		if(MQTT_SubscribeTopic(MQTT_SUBSCRIBE_TOPIC,0,1) != 0)
		{
			user_main_info("ESP8266 ОБЛАЧНЫЙ СЕРВЕР MQTT УСПЕШНО ПОДПИСАЛСЯ НА ЭТУ ТЕМУ！\r\n");
		}
		else Enter_ErrorMode(4);
	}
}

/******************************  STM32 MQTT КОД  *****************************/

//ФУНКЦИЯ ИНИЦИАЛИЗАЦИИ MQTT
void ES8266_MQTT_Init(void)
{
	uint8_t status=0;

	//ИНИЦИАЛИЗАЦИЯ
	if(ESP8266_Init())
	{
		user_main_info("ESP8266 ИНЦИАЛИЗАЦИЯ ПРОШЛА УСПЕШНО！\r\n");
		status++;
	}
	else Enter_ErrorMode(0);

	//ПОДЛКЮЧЕНИЕ К ТОЧКЕ ДОСТУПА
	if(status==1)
	{
		if(ESP8266_ConnectAP(WIFI_NAME,WIFI_PASSWD))
		{
			user_main_info("ESP8266УСПЕШНО ПОДКЛЮЧИЛСЯ К ТОЧКЕ ДОСТУПА！\r\n");
			status++;
		}
		else Enter_ErrorMode(1);
	}

	//ПОДКЛЮЧЕНИЕ К ОБЛАЧНОМУ СЕРВЕРУ
	if(status==2)
	{
		if(ESP8266_ConnectServer("TCP",MQTT_BROKERADDRESS,1883)!=0)
		{
			user_main_info("ESP8266УСПЕШНОЕ ПОДКЛЮЧЕНИЕ К ОБЛАЧНОМУ СЕРВЕРУ！\r\n");
			status++;
		}
		else Enter_ErrorMode(2);
	}

	//ВХОД В MQTT
	if(status==3)
	{
		if(MQTT_Connect(MQTT_CLIENTID, MQTT_USARNAME, MQTT_PASSWD) != 0)
		{
			user_main_info("ESP8266 ОБЛАЧНЫЙ СЕРВЕР MQTT УСПЕШОН ВОШЕЛ В СИСТЕМУ！\r\n");
			status++;
		}
		else Enter_ErrorMode(3);
	}

	//ПОДПИСКА НА ТЕМЫ
	if(status==4)
	{
		if(MQTT_SubscribeTopic(MQTT_SUBSCRIBE_TOPIC,0,1) != 0)
		{
			user_main_info("ESP8266 ОБЛАЧНЫЙ СЕРВЕР УСПЕШНО ПОДПИСАЛСЯ НА ЭТУ ТЕМУ！\r\n");
		}
		else Enter_ErrorMode(4);
	}
}

//ОТЧЕТ О СОСТОЯНИИ MCU
void STM32DHT11_StatusReport(void)
{
	//ПОЛУЧЕНИЕ ИНФОРМАЦИИ О ТЕМПЕРАТУРЕ И ВЛАЖНОСТИ
	uint8_t temperature;
	uint8_t humidity;
	uint8_t get_times;

	// 获ПОЛУЧЕНИЕ ИНФОРМАЦИИ О ТЕМПЕРАТУРЕ И ВЛАЖНОСТИ И РАСПЕЧАТАТЬ ЕЕ С ПОМОЩЬЮ ПОСЛЕДОВАТЕЛЬНОГО ПОРТА, ПОЛУЧИТЬ ЕЕ ДЕСЯТЬ РАЗ,ПОКА ОНА УСПЕШНО НЕ ВЫСКОЧИТ
	for(get_times=0;get_times<10;get_times++)
	{
		if(!dht11Read(&temperature, &humidity))//Read DHT11 Value
		{
			user_main_info("temperature=%d,humidity=%d \n",temperature,humidity);
			break;
		}
	}

	//ОТЧЕТ О ДАННЫХ ОДИН РАЗ
	MQTT_PublishData(MQTT_PUBLISH_TOPIC,mqtt_message,0);
}

char temp_str[30];    // ВРЕМЕННАЯ ПОДСТРОКА
void ReadStrUnit(char * str,char *temp_str,int idx,int len)  // ПОЛУЧЕНИЕ ВРЕМЕННОЙ ВЛОЖЕННОЙ СТРОКИ, РАВНУЮ ДЛИНЕ ВЛОЖЕННОЙ СТРОКИ ИЗ РОДИТЕЛЬСКОЙ СТРОКИ
{
    int index;
    for(index = 0; index < len; index++)
    {
        temp_str[index] = str[idx+index];
    }
    temp_str[index] = '\0';
}
int GetSubStrPos(char *str1,char *str2)
{
    int idx = 0;
    int len1 = strlen(str1);
    int len2 = strlen(str2);

    if( len1 < len2)
    {
        //printf("error 1 \n"); // ДОЧЕРНЯЯ СТРОКА ДЛИННЕЕ РОДИТЕЛЬСКОЙ СТРОКИ
        return -1;
    }

    while(1)
    {
        ReadStrUnit(str1,temp_str,idx,len2);    // НЕПРЕРЫВНО ПОЛУЧАТЬ ОБНОВЛЕНИЕ ВРЕМЕННОЙ ВЛОЖЕННОЙ СТРОКИ ИЗ ПОЗИЦИИ IDX РОДИТЕЛЬСКОЙ СТРОКИ
        if(strcmp(str2,temp_str)==0)break;      // ЕСЛИ ВРЕМЕННАЯ ПОДСТРОКА И ПОДСТРОКА СОВПАДАЮТ, ЦИКЛ ЗАВЕРШАЕТСЯ
        idx++;                                  // ИЗМЕНЕНИЕ ПОЛОЖЕНИЯ ВЗЯТИЕ ВРЕМЕННЫХ ВЛОЖЕННЫХ СТРОК ИЗ РОДИТЕЛЬСКОЙ СТРОКИ
        if(idx>=len1)return -1;                 // ЕСЛИ IDX ПРЕВЫСИЛ ДЛИНУ ВНУТРЕННЕЙ СТРОКИ, ВНУТРЕННЯЯ СТРОКА НЕ СОДЕРЖИТЬ ДОЧЕРНЕЙ СТРОКИ
    }

    return idx;    // ВОЗВРАЩАЕТ ПОЗИЦИЮ ПЕРВОГО СИМВОЛА ВЛОЖЕННОЙ СТРОКИ В РОДИТЕЛЬСКОЙ СТРОКЕ
}

//ОБРАБАТЫВАТЬ СООБЩЕНИЯ, ОТПРАВЛЕННЫЕ MQTT
void deal_MQTT_message(uint8_t* buf,uint16_t len)
{
	uint8_t data[512];
	uint16_t data_len = len;
	for(int i=0;i<data_len;i++)
	{
		data[i] = buf[i];
		HAL_UART_Transmit(&huart1,&data[i],1,100);
	}
	memset(usart2_rxbuf,0,sizeof(usart2_rxbuf)); //ОЧИСТИТЬ БУФЕР ПРИЕМА
	usart2_rxcounter=0;
	//user_main_info("MQTT ПОЛУЧИЛ СООБЩЕНИЕ, ДЛИНА ДАННЫХ=%d \n",data_len);

	//УЗНАЙТЕ ВКЛЮЧЕН ЛИ КРАСНЫЙ ИНДИКАТОР
	int i = GetSubStrPos((char*)data,"LEDR");
	if( i>0 )
	{
		uint8_t ledr_status = data[i+6]-'0';
		HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		if(ledr_status)
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
	}

	//УЗНАЙТЕ ВКЛЮЧЕН ЛИ ЗЕЛЕНЫЙ ИНДИКАТОР
	i = GetSubStrPos((char*)data,"LEDG");
	if( i>0 )
	{
		uint8_t ledr_status = data[i+6]-'0';
		HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		if(ledr_status)
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
	}

	//УЗНАЙТЕ ВКЛЮЧЕН ЛИ СИНИЙ ИНДИКАТОР
	i = GetSubStrPos((char*)data,"LEDB");
	if( i>0 )
	{
		uint8_t ledr_status = data[i+6]-'0';
		HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_G_GPIO_Port,LED_G_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
		if(ledr_status)
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(LED_B_GPIO_Port,LED_B_Pin,GPIO_PIN_SET);
	}

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
