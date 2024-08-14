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
#include "can_driver.h"
#include "config.h"
#include "mpu6050.h"
#include "lim.h"
#include "pressure.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_SEM_COUNT 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for PressureSensing */
osThreadId_t PressureSensingHandle;
const osThreadAttr_t PressureSensing_attributes = {
  .name = "PressureSensing",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TempSensing */
osThreadId_t TempSensingHandle;
const osThreadAttr_t TempSensing_attributes = {
  .name = "TempSensing",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for IMUProcessing */
osThreadId_t IMUProcessingHandle;
const osThreadAttr_t IMUProcessing_attributes = {
  .name = "IMUProcessing",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CAN_Sensor_TX */
osThreadId_t CAN_Sensor_TXHandle;
const osThreadAttr_t CAN_Sensor_TX_attributes = {
  .name = "CAN_Sensor_TX",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CAN_IMU_TX */
osThreadId_t CAN_IMU_TXHandle;
const osThreadAttr_t CAN_IMU_TX_attributes = {
  .name = "CAN_IMU_TX",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Processed_Temp_Sensor_Data_Q */
osMessageQueueId_t Processed_Temp_Sensor_Data_QHandle;
const osMessageQueueAttr_t Processed_Temp_Sensor_Data_Q_attributes = {
  .name = "Processed_Temp_Sensor_Data_Q"
};
/* Definitions for Processed_IMU_Data_Q */
osMessageQueueId_t Processed_IMU_Data_QHandle;
const osMessageQueueAttr_t Processed_IMU_Data_Q_attributes = {
  .name = "Processed_IMU_Data_Q"
};
/* Definitions for Processed_Pressure_Sensor_Data_Q */
osMessageQueueId_t Processed_Pressure_Sensor_Data_QHandle;
const osMessageQueueAttr_t Processed_Pressure_Sensor_Data_Q_attributes = {
  .name = "Processed_Pressure_Sensor_Data_Q"
};
/* Definitions for TempSem */
osSemaphoreId_t TempSemHandle;
const osSemaphoreAttr_t TempSem_attributes = {
  .name = "TempSem"
};
/* Definitions for PressureSem */
osSemaphoreId_t PressureSemHandle;
const osSemaphoreAttr_t PressureSem_attributes = {
  .name = "PressureSem"
};
/* Definitions for IMUSem */
osSemaphoreId_t IMUSemHandle;
const osSemaphoreAttr_t IMUSem_attributes = {
  .name = "IMUSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void process_pressure_data(void *argument);
void process_temp_data(void *argument);
void process_IMU_data(void *argument);
void process_CAN_sensor_data(void *argument);
void process_CAN_IMU_data(void *argument);

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
  TempSemHandle = osSemaphoreNew(1, 0, &TempSem_attributes);

  /* creation of PressureSem */
  PressureSemHandle = osSemaphoreNew(1, 0, &PressureSem_attributes);

  /* creation of IMUSem */
  IMUSemHandle = osSemaphoreNew(1, 0, &IMUSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of Processed_Temp_Sensor_Data_Q */
  Processed_Temp_Sensor_Data_QHandle = osMessageQueueNew (11, sizeof(uint64_t), &Processed_Temp_Sensor_Data_Q_attributes);

  /* creation of Processed_IMU_Data_Q */
  Processed_IMU_Data_QHandle = osMessageQueueNew (9, sizeof(uint64_t), &Processed_IMU_Data_Q_attributes);

  /* creation of Processed_Pressure_Sensor_Data_Q */
  Processed_Pressure_Sensor_Data_QHandle = osMessageQueueNew (1, sizeof(uint64_t), &Processed_Pressure_Sensor_Data_Q_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of PressureSensing */
  PressureSensingHandle = osThreadNew(process_pressure_data, NULL, &PressureSensing_attributes);

  /* creation of TempSensing */
  TempSensingHandle = osThreadNew(process_temp_data, NULL, &TempSensing_attributes);

  /* creation of IMUProcessing */
  IMUProcessingHandle = osThreadNew(process_IMU_data, NULL, &IMUProcessing_attributes);

  /* creation of CAN_Sensor_TX */
  CAN_Sensor_TXHandle = osThreadNew(process_CAN_sensor_data, NULL, &CAN_Sensor_TX_attributes);

  /* creation of CAN_IMU_TX */
  CAN_IMU_TXHandle = osThreadNew(process_CAN_IMU_data, NULL, &CAN_IMU_TX_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_process_pressure_data */
/**
  * @brief  Function implementing the PressureSensing thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_process_pressure_data */
void process_pressure_data(void *argument)
{
  /* USER CODE BEGIN process_pressure_data */
	  uint8_t pressure = 0;
	  Data_Type_Value_Pairing_t temp_queue_pair;
  /* Infinite loop */
  for(;;)
  {
	  //poll pressure sensor
	  pressure = poll_pressure_sensor();

	  //create data pairing struct
	  temp_queue_pair.data_type = PRESSURE;
	  temp_queue_pair.data_value = pressure;

	  //Push data struct to Sensor data queue
	  osMessageQueuePut(Processed_Pressure_Sensor_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	  //Release semaphore to signal pressure data ready in Queue
	  osSemaphoreRelease(PressureSemHandle);

	  osDelay(1);
  }
  /* USER CODE END process_pressure_data */
}

/* USER CODE BEGIN Header_process_temp_data */
/**
* @brief Function implementing the TempSensing thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_process_temp_data */
void process_temp_data(void *argument)
{
  /* USER CODE BEGIN process_temp_data */
	  uint8_t lim_temps[NUM_THERM_TOTAL];
	  Data_Type_Value_Pairing_t temp_queue_pair;
  /* Infinite loop */
  for(;;)
  {
	  //poll LIM thermistors
	  get_lim_data(lim_temps);

	  //create data pairing structs and push them to Sensor data queue
	  temp_queue_pair.data_type = LIM_ONE_TEMP_ONE;
	  temp_queue_pair.data_value = lim_temps[0];
	  osMessageQueuePut(Processed_Temp_Sensor_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	  temp_queue_pair.data_type = LIM_ONE_TEMP_TWO;
	  temp_queue_pair.data_value = lim_temps[1];
	  osMessageQueuePut(Processed_Temp_Sensor_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	  temp_queue_pair.data_type = LIM_ONE_TEMP_THREE;
	  temp_queue_pair.data_value = lim_temps[2];
	  osMessageQueuePut(Processed_Temp_Sensor_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	  temp_queue_pair.data_type = LIM_TWO_TEMP_ONE;
	  temp_queue_pair.data_value = lim_temps[3];
	  osMessageQueuePut(Processed_Temp_Sensor_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	  temp_queue_pair.data_type = LIM_TWO_TEMP_TWO;
	  temp_queue_pair.data_value = lim_temps[4];
	  osMessageQueuePut(Processed_Temp_Sensor_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	  temp_queue_pair.data_type = LIM_TWO_TEMP_THREE;
	  temp_queue_pair.data_value = lim_temps[5];
	  osMessageQueuePut(Processed_Temp_Sensor_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	  //Release semaphore to signal Temperature data ready in Queue
	  osSemaphoreRelease(TempSemHandle);

	  osDelay(1);
  }
  /* USER CODE END process_temp_data */
}

/* USER CODE BEGIN Header_process_IMU_data */
/**
* @brief Function implementing the IMUProcessing thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_process_IMU_data */
void process_IMU_data(void *argument)
{
  /* USER CODE BEGIN process_IMU_data */
	int16_t x_accel = 0;
	int16_t y_accel = 0;
	int8_t x_gyro = 0;
	int8_t y_gyro = 0;
	int8_t z_gyro = 0;
	Data_Type_Value_Pairing_t temp_queue_pair;
  /* Infinite loop */
  for(;;)
  {
	//poll IMU
	MPU6050_Read_Accel(&x_accel, &y_accel);
	MPU6050_Read_Gyro(&x_gyro, &y_gyro, &z_gyro);

	//create data pairing structs and push them to IMU data queue
	temp_queue_pair.data_type = X_ACCEL;
	temp_queue_pair.data_value = x_accel;
	osMessageQueuePut(Processed_IMU_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	temp_queue_pair.data_type = Y_ACCEL;
	temp_queue_pair.data_value = y_accel;
	osMessageQueuePut(Processed_IMU_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	temp_queue_pair.data_type = X_GYRO;
	temp_queue_pair.data_value = x_gyro;
	osMessageQueuePut(Processed_IMU_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	temp_queue_pair.data_type = Y_GYRO;
	temp_queue_pair.data_value = y_gyro;
	osMessageQueuePut(Processed_IMU_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	temp_queue_pair.data_type = Z_GYRO;
	temp_queue_pair.data_value = z_gyro;
	osMessageQueuePut(Processed_IMU_Data_QHandle, &temp_queue_pair, 0, osWaitForever);

	//Release semaphore to signal IMU data ready in Queue
	osSemaphoreRelease(IMUSemHandle);

	osDelay(1);
  }
  /* USER CODE END process_IMU_data */
}

/* USER CODE BEGIN Header_process_CAN_sensor_data */
/**
* @brief Function implementing the CAN_Sensor_TX thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_process_CAN_sensor_data */
void process_CAN_sensor_data(void *argument)
{
  /* USER CODE BEGIN process_CAN_sensor_data */
	uint8_t error_code = 0;
	Data_Type_Value_Pairing_t received_data;

	//Initialize the tx CAN frame
	CAN_Frame_t tx_frame = CAN_frame_init(&hcan3, SENSOR_BOARD_1);
  /* Infinite loop */
  for(;;)
  {
	  //CAN frame Temperature data entry
	  //Acquire Temperature semaphore to ensure data is ready in queue for transfer
	  osSemaphoreAcquire(TempSemHandle, osWaitForever);

	  //Add 6 entries of the Temp data pairings into the CAN message frame (6 different data pairing are added consecutively at a time by process_temp_data)
	  for (uint8_t i = 0; i < 6; i++) {
		  if (osMessageQueueGet(Processed_Temp_Sensor_Data_QHandle, &received_data, NULL, osWaitForever) == osOK)
		  {
			  CAN_set_segment(&tx_frame, received_data.data_type, received_data.data_value);
		  }
	  }

	  //CAN frame Pressure data entry
	  //Acquire Pressure semaphore to ensure data is ready in queue for transfer
	  osSemaphoreAcquire(PressureSemHandle, osWaitForever);

	  //Add Pressure data pairing into the CAN message frame
	  if (osMessageQueueGet(Processed_Pressure_Sensor_Data_QHandle, &received_data, NULL, osWaitForever) == osOK)
	  {
		  CAN_set_segment(&tx_frame, received_data.data_type, received_data.data_value);
	  }

	  //Add error code data pairing to the CAN frame
	  CAN_set_segment(&tx_frame, SENSORS_ERROR_CODE_1, error_code);

	  //Send CAN message
	  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
		  CAN_send_frame(tx_frame);
	  }
	  osDelay(1);
  }
  /* USER CODE END process_CAN_sensor_data */
}

/* USER CODE BEGIN Header_process_CAN_IMU_data */
/**
* @brief Function implementing the CAN_IMU_TX thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_process_CAN_IMU_data */
void process_CAN_IMU_data(void *argument)
{
  /* USER CODE BEGIN process_CAN_IMU_data */
	  uint8_t error_code = 0;
	  Data_Type_Value_Pairing_t received_data;

	  //Initialize the tx CAN frame
	  CAN_Frame_t imu_frame = CAN_frame_init(&hcan3, SENSOR_BOARD_2);
  /* Infinite loop */
  for(;;)
  {
	  //Acquire IMU semaphore to ensure data is ready in queue for transfer
	  osSemaphoreAcquire(IMUSemHandle, osWaitForever);

	  //Add 5 entries of the IMU data pairings into the CAN message frame (5 different data pairing are added consecutively at a time by process_IMU_data)
	  for (uint8_t i = 0; i < 5; i++) {
		  if (osMessageQueueGet(Processed_IMU_Data_QHandle, &received_data, NULL, osWaitForever) == osOK)
		  {
			  CAN_set_segment(&imu_frame, received_data.data_type, received_data.data_value);
		  }
	  }

	  //Add error code data pairing to the CAN frame
	  CAN_set_segment(&imu_frame, SENSORS_ERROR_CODE_2, error_code);

	  //Send CAN message
	  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
		  CAN_send_frame(imu_frame);
	  }
	  osDelay(1);
  }
  /* USER CODE END process_CAN_IMU_data */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

