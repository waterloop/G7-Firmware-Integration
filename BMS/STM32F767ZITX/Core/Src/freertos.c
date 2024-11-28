/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "temp_sensing.h"
#include "can_driver.h"
#include "config.h"

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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for temp_sense */
osThreadId_t temp_senseHandle;
const osThreadAttr_t temp_sense_attributes = {
  .name = "temp_sense",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for can_transmitter */
osThreadId_t can_transmitterHandle;
const osThreadAttr_t can_transmitter_attributes = {
  .name = "can_transmitter",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for temp_sensor_data */
osMessageQueueId_t temp_sensor_dataHandle;
const osMessageQueueAttr_t temp_sensor_data_attributes = {
  .name = "temp_sensor_data"
};
/* Definitions for TempSem */
osSemaphoreId_t TempSemHandle;
const osSemaphoreAttr_t TempSem_attributes = {
  .name = "TempSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTemp_Sense(void *argument);
void StartCan_Transmitter(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of TempSem */
  TempSemHandle = osSemaphoreNew(2, 2, &TempSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of temp_sensor_data */
  temp_sensor_dataHandle = osMessageQueueNew (16, sizeof(uint32_t), &temp_sensor_data_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of temp_sense */
  temp_senseHandle = osThreadNew(StartTemp_Sense, NULL, &temp_sense_attributes);

  /* creation of can_transmitter */
  can_transmitterHandle = osThreadNew(StartCan_Transmitter, NULL, &can_transmitter_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTemp_Sense */
/**
* @brief Function implementing the temp_sense thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTemp_Sense */
void StartTemp_Sense(void *argument)
{
  /* USER CODE BEGIN StartTemp_Sense */
	uint32_t adc_data[NUM_MUX];
	uint32_t temp_average[NUM_MUX] = {0}; //average temp from the previous iterations
	uint32_t temp_data[NUM_SAMPLES][NUM_MUX] = {0};
  /* Infinite loop */
  for(;;)
  {
	//Measures average thermistor temperature of each MUX
	measureTempADC(temp_average, adc_data, temp_data);
	osMessageQueuePut(temp_sensor_dataHandle, temp_average, 0, osWaitForever);
	osSemaphoreRelease(TempSemHandle);
    osDelay(1);
  }
  /* USER CODE END StartTemp_Sense */
}

/* USER CODE BEGIN Header_StartCan_Transmitter */
/**
* @brief Function implementing the can_transmitter thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCan_Transmitter */
void StartCan_Transmitter(void *argument)
{
  /* USER CODE BEGIN StartCan_Transmitter */

	//create buffer int array/ use to pull data
	uint32_t temp_buffer[NUM_MUX] = {0};
	//Initialize the tx CAN frame, use this to send out data (frame is a package of mail)
	CAN_Frame_t tx_frame = CAN_frame_init(&hcan2, BMS_BOARD);
	//Error code initialization
	uint8_t error_code = 0;
  /* Infinite loop */
  for(;;)
  {
	//semaphore acquire (counter--)
	osSemaphoreAcquire(TempSemHandle, osWaitForever);
	//receives the message from the transmitter
	//grab value from os queue and put into buffer
	osMessageQueueGet(temp_sensor_dataHandle, temp_buffer, NULL, osWaitForever);

	//CAN_set_segment for each value in the temp buffer
	CAN_set_segment(&tx_frame, MUX1_TEMP , temp_buffer[0]);
	CAN_set_segment(&tx_frame, MUX2_TEMP , temp_buffer[1]);
	CAN_set_segment(&tx_frame, MUX3_TEMP , temp_buffer[2]);
	CAN_set_segment(&tx_frame, MUX4_TEMP , temp_buffer[3]);
	CAN_set_segment(&tx_frame, MUX5_TEMP , temp_buffer[4]);
	CAN_set_segment(&tx_frame, MUX6_TEMP , temp_buffer[5]);

	//error code something
	CAN_set_segment(&tx_frame, BMS_ERROR_CODE, error_code);

	//Send data to RPI
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2)) {
    	CAN_send_frame(tx_frame);
    }

	osDelay(1);
  }
  /* USER CODE END StartCan_Transmitter */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

