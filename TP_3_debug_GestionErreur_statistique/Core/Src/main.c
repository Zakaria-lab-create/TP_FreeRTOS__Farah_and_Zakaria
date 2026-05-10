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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "shell.h"

/*
#define STACK_SIZE 256
#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 2
#define TASK1_DELAY 1
#define TASK2_DELAY 2
*/
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

// SemaphoreHandle_t xSemaphore = NULL;

//TaskHandle_t taskTakeHandle = NULL;

// QueueHandle_t xQueue = NULL;

QueueHandle_t shellQueue;

SemaphoreHandle_t xPrintMutex;

uint32_t led_period = 0;

TaskHandle_t ledTaskHandle = NULL;

BaseType_t ret;

char spam_msg[64] = "hello";
uint32_t spam_count = 0;

QueueHandle_t xDataQueue;
SemaphoreHandle_t xBinarySemaphore;
SemaphoreHandle_t xUartMutex;

typedef struct
{
    uint32_t counter;
    uint32_t value;
} SensorMessage_t;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

int sh_stats(int argc, char **argv);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch) {
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}



void led_task(void *arg)
{

//Suspend() et resume()


    for (;;)
    {
        if (led_period == 0)
        {
            HAL_GPIO_WritePin(GPIOI, GPIO_PIN_1, GPIO_PIN_RESET);
            vTaskDelay(10);
        }
        else
        {
            HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_1);
            vTaskDelay(pdMS_TO_TICKS(led_period));
        }
    }
}

void task_overflow(void *arg)
{
    // Grosse variable locale = explosion de pile
    uint8_t big_buffer[500];

    for (;;)
    {
        big_buffer[0] = 42; // juste pour éviter l’optimisation
        vTaskDelay(10);
    }
}

static void uart_print(const char *msg)
{
    if (xUartMutex != NULL)
    {
        if (xSemaphoreTake(xUartMutex, portMAX_DELAY) == pdTRUE)
        {
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            xSemaphoreGive(xUartMutex);
        }
    }
}

void boom()
{
    uint8_t buffer[200];
    boom(); // récursion infinie
}

void task_overflow2(void *arg)
{
    boom();
}


void spam_task(void *arg)
{
    for (;;)
    {
        if (spam_count > 0)
        {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);

            printf("%s\r\n", spam_msg);
            xSemaphoreGive(xPrintMutex);

            spam_count--;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void StartTaskSensor(void *argument)
{
    SensorMessage_t msg;
    uint32_t count = 0;
    char buffer[80];

    for (;;)
    {
        msg.counter = count++;
        msg.value = msg.counter * 10;

        if (xQueueSend(xDataQueue, &msg, pdMS_TO_TICKS(50)) == pdPASS)
        {
            snprintf(buffer, sizeof(buffer),
                     "[TaskSensor] Sent -> counter=%lu value=%lu\r\n",
                     msg.counter, msg.value);
            uart_print(buffer);
        }
        else
        {
            uart_print("[TaskSensor] Queue full\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ---------------- Task 2 : consomme les données ---------------- */
void StartTaskProcess(void *argument)
{
    SensorMessage_t rxMsg;
    char buffer[80];

    for (;;)
    {
        if (xQueueReceive(xDataQueue, &rxMsg, portMAX_DELAY) == pdPASS)
        {
            snprintf(buffer, sizeof(buffer),
                     "[TaskProcess] Received -> counter=%lu value=%lu\r\n",
                     rxMsg.counter, rxMsg.value);
            uart_print(buffer);
        }
    }
}

int sh_stats(int argc, char **argv)
{
    char buffer[512];

    printf("=== Liste des tâches ===\r\n");
    vTaskList(buffer);
    printf("%s\r\n", buffer);

    printf("=== Statistiques CPU ===\r\n");
    vTaskGetRunTimeStats(buffer);
    printf("%s\r\n", buffer);

    return 0;
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

  //Mutex :

  xPrintMutex = xSemaphoreCreateMutex();
  configASSERT(xPrintMutex != NULL);

  shellQueue = xQueueCreate(64, sizeof(char));
  configASSERT(shellQueue != NULL);

  shell_init();

  xDataQueue = xQueueCreate(8, sizeof(SensorMessage_t));
  xBinarySemaphore = xSemaphoreCreateBinary();
  xUartMutex = xSemaphoreCreateMutex();

  if ((xDataQueue == NULL) || (xBinarySemaphore == NULL) || (xUartMutex == NULL))
  {
      Error_Handler();
  }

  //gestion d erreur
  //3.1

  //Création des tâches bidons :
/*  for (int i = 0; i < 50; i++) {
      char name[16];
      sprintf(name, "T%d", i);

      BaseType_t ret = xTaskCreate(spam_task, name, 256, NULL, 1, NULL);//On a 256 mots donc en octet ; on a 256 x4

      if (ret != pdPASS) {
          printf("Erreur: plus de memoire pour creer la tache %d\n", i);
          Error_Handler();
      }
  }*/


  shell_add('t', sh_stats, "Affiche les stats FreeRTOS");
  xTaskCreate(shell_task, "shell_task", 256, NULL, 1, NULL);//On a 256 mots donc en octet ; on a 256 x4
  xTaskCreate(StartTaskProcess,    "TaskProcess",    256, NULL, 2, NULL);
  xTaskCreate(spam_task, "Spam_task", 256, NULL, 1, NULL);
  //xTaskCreate(task_overflow2, "overflow", 256, NULL, 1, NULL);
  //xTaskCreate(task_overflow, "overflow2", 256, NULL, 1, NULL);

  //shell_add('t', sh_stats, "Affiche les stats FreeRTOS");

  vTaskStartScheduler();
  // tasks :

/*   xTaskCreate(shell_task, "shell", 512, NULL, 3, NULL);

   xTaskCreate(led_task, "led", 256, NULL, 2, &ledTaskHandle);

   xTaskCreate(spam_task, "spam", 256, NULL, 2, NULL);*/



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

	 // blinkLed();
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


/*void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();

    printf("Erreur : stack overflow dans la tâche %s\n", pcTaskName);

    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_1, GPIO_PIN_SET);

    while (1) {}
}*/



void configureTimerForRunTimeStats(void)
{
    MX_TIM2_Init();                 // Initialise TIM2
    HAL_TIM_Base_Start(&htim2);     // Démarre le timer
}


unsigned long getRunTimeCounterValue(void)
{
	 return __HAL_TIM_GET_COUNTER(&htim2);
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
