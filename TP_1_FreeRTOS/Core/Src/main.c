/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "cmsis_os.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define STACK_SIZE 256
#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 2
#define TASK1_DELAY 1
#define TASK2_DELAY 2
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

SemaphoreHandle_t xSemaphore = NULL;
SemaphoreHandle_t xPrintMutex = NULL;

TaskHandle_t taskTakeHandle = NULL;

QueueHandle_t xQueue = NULL;

SemaphoreHandle_t xPrintMutex;

BaseType_t ret;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch) {
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}



void blinkLed()
{
	HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_1);
	HAL_Delay(500);
}

void User_Button()
{
    if (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_11) == GPIO_PIN_SET)
    {
        HAL_GPIO_WritePin(GPIOI, GPIO_PIN_1, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOI, GPIO_PIN_1, GPIO_PIN_RESET);
    }
}

void led_blink_task(void* unused)
{
	//char msg[100] = "Led toggled";
	for(;;)
	{
		HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_1);
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
		printf("\r\n");
		printf("Toggled\r\n");
		xSemaphoreGive(xPrintMutex);
		//printf("\r\n");
		vTaskDelay(100/portTICK_PERIOD_MS);//La LED va clignonter à chaque 100 ms
	}
}

/*void taskGive(void *unused)
{
    for (;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("\r\n");
        printf("[tak Give] Avant give\r\n");
        xSemaphoreGive(xPrintMutex);

        xSemaphoreGive(xSemaphore);

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[task Give] Après give\r\n");
        xSemaphoreGive(xPrintMutex);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}*/

/*void taskTake(void *unused)
{
    for (;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[task Take] Avant take\r\n");
        xSemaphoreGive(xPrintMutex);

        xSemaphoreTake(xSemaphore, portMAX_DELAY);

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[task Take] Après take\r\n");
        xSemaphoreGive(xPrintMutex);
    }
}*/

//Fonction give gestion d'erreur
/*void taskGive(void *unused)
{
    static uint32_t delay = 100;  // commence à 100 ms

    for (;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("\r\n[task Give] Avant give (delay = %lu ms)\r\n", delay);
        xSemaphoreGive(xPrintMutex);

        xSemaphoreGive(xSemaphore);

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[task Give] Après give\r\n");
        xSemaphoreGive(xPrintMutex);

        vTaskDelay(pdMS_TO_TICKS(delay));

        delay += 100;  // on ajoute 100 ms à chaque tour
    }
}*/

//Fonction give pour notifications
/*void taskGive(void *unused)
{
    static uint32_t delay = 100;

    for (;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("\r\n[Give] Avant notification (delay = %lu ms)\r\n", delay);
        xSemaphoreGive(xPrintMutex);

        // Envoi de la notification à taskTake
        xTaskNotifyGive(taskTakeHandle);

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[Give] Après notification\r\n");
        xSemaphoreGive(xPrintMutex);

        vTaskDelay(pdMS_TO_TICKS(delay));
        delay += 100;  // pour tester l’erreur
    }
}*/


//Fonction give pour queue
/*void taskGive(void *unused)
{
    uint32_t tickValue;

    for (;;)
    {
         Lecture du timer RTOS
        tickValue = xTaskGetTickCount();

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("\r\n[Give] Envoi tick = %lu\r\n", tickValue);
        xSemaphoreGive(xPrintMutex);

         Envoi dans la queue
        xQueueSend(xQueue, &tickValue, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}*/


//Fonction take de la gestion d'erreur
/*void taskTake(void *unused)
{
    for (;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("\r\n[task Take] Avant take (attente sémaphore)\r\n");
        xSemaphoreGive(xPrintMutex);

        // Timeout de 1000 ms
        if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[task Take] Après take (sémaphore reçu)\r\n");
            xSemaphoreGive(xPrintMutex);
        }
        else
        {
            // ERREUR : sémaphore non reçu dans les temps
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[task Take] ERREUR : sémaphore non reçu en 1 seconde ! RESET...\r\n");
            xSemaphoreGive(xPrintMutex);

            // Reset logiciel du STM32
            NVIC_SystemReset();
        }
    }
}*/


//Fonction take pour notifications
/*void taskTake(void *unused)
{
    for (;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("\r\n[Take] Avant attente notification\r\n");
        xSemaphoreGive(xPrintMutex);

        // Attend une notification pendant 1 seconde
        uint32_t result = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        if (result > 0)
        {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[Take] Notification reçue !\r\n");
            xSemaphoreGive(xPrintMutex);
        }
        else
        {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[Take] ERREUR : pas de notification en 1 seconde ! RESET...\r\n");
            xSemaphoreGive(xPrintMutex);

            NVIC_SystemReset();
        }
    }
}*/


//Fonction take pour queue
/*void taskTake(void *unused)
{
    uint32_t receivedTick;

    for (;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[Take] En attente de données...\r\n");
        xSemaphoreGive(xPrintMutex);

         Réception avec timeout de 1 seconde
        if (xQueueReceive(xQueue,
                           &receivedTick,
                           pdMS_TO_TICKS(1000)) == pdPASS)
        {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[Take] Tick reçu = %lu\r\n", receivedTick);
            xSemaphoreGive(xPrintMutex);
        }
        else
        {
             Erreur : aucune donnée reçue
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[ERREUR] Timeout queue -> Reset\r\n");
            xSemaphoreGive(xPrintMutex);

            vTaskDelay(pdMS_TO_TICKS(10));
            NVIC_SystemReset();
        }
    }
}*/

//Problème de bug :
/*void task_bug(void * pvParameters)
{
    int delay = (int) pvParameters;

    for(;;)
    {
        printf("Je suis %s et je m'endors pour %d ticks\r\n",
               pcTaskGetName(NULL), delay);

        vTaskDelay(delay);
    }
}*/

//Solution contre le bug :
void task_bug(void * pvParameters)
{
    int delay = (int) pvParameters;

    for(;;)
    {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);

        printf("Je suis %s et je m'endors pour %d ticks\r\n",
               pcTaskGetName(NULL), delay);

        xSemaphoreGive(xPrintMutex);

        vTaskDelay(delay);
    }
}


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
  /* USER CODE BEGIN 2 */

  //Mutex :

  xPrintMutex = xSemaphoreCreateMutex();
  configASSERT(xPrintMutex != NULL);

  // semaphore :
  xSemaphore = xSemaphoreCreateBinary();


  xPrintMutex = xSemaphoreCreateMutex();


   if (xSemaphore == NULL || xPrintMutex == NULL)
   {
       printf("Erreur : impossible de créer le sémaphore/mutex\r\n");
       while (1);
   }

   //queue

xQueue = xQueueCreate(5, sizeof(uint32_t));  // queue de 5 éléments de 32 bits

   if (xQueue == NULL)
   {
       printf("Erreur : impossible de créer la queue\r\n");
       while (1);
   }

  // tasks :

  //xTaskCreate(led_blink_task, "blink", 128, NULL, 1, NULL);

/*  xTaskCreate(taskGive, "give", 256, NULL, 2, NULL);
  xTaskCreate(taskTake, "take", 256, NULL, 3, NULL);*/

  //xTaskCreate(taskTake, "take", 256, NULL, 1, &taskTakeHandle);


  //Test de bug :

  ret = xTaskCreate(task_bug, "Tache 1", STACK_SIZE,
                    (void *) TASK1_DELAY, TASK1_PRIORITY, NULL);
  configASSERT(pdPASS == ret);

  ret = xTaskCreate(task_bug, "Tache 2", STACK_SIZE,
                    (void *) TASK2_DELAY, TASK2_PRIORITY, NULL);
  configASSERT(pdPASS == ret);

  vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  //blinkLed();
	  //User_Button();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
#ifdef USE_FULL_ASSERT
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
