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
#include "can_driver.h"
#include "config.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {

	uint32_t direction;
	uint32_t speed;
	uint32_t error_code;
	uint32_t voltage;
	uint32_t current;
	uint32_t motor_temp;
	uint32_t controller_temp;

} Motor_Controller_t;

typedef enum {
	NEUTRAL,
	FORWARD,
	REVERSE
} DrivingDirection;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define MAX_SEM_COUNT 1

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* TID and attribute definitions */

osThreadId_t MC_ReceiveStartTaskHandle;
const osThreadAttr_t MC_ReceiveStartTaskAttr = {
		.name = "MC_ReceiveStartThread",
		.stack_size = 128 * 4,
		.priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t MC_ReceiveEndTaskHandle;
const osThreadAttr_t MC_ReceiveEndTaskAttr = {
		.name = "MC_ReceiveEndThread",
		.stack_size = 128 * 4,
		.priority = (osPriority_t) osPriorityHigh,
};


osThreadId_t MC_SendStartTaskHandle;
const osThreadAttr_t MC_SendStartTaskAttr = {
		.name = "MC_SendStartThread",
		.stack_size = 128*4,
		.priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t MC_SendEndTaskHandle;
const osThreadAttr_t MC_SendEndTaskAttr = {
		.name = "MC_SendEndThread",
		.stack_size = 128*4,
		.priority = (osPriority_t) osPriorityHigh,
};

/* Queue definitions for CAN frames */

osMessageQueueId_t start_frame_q;
osMessageQueueId_t end_frame_q;


/* Definitions for start and end CAN frame semaphores */
osSemaphoreId_t MCSemStartHandle;
const osSemaphoreAttr_t MCSemStart_attributes = {
  .name = "MCSemStart"
};

osSemaphoreId_t MCSemEndHandle;
const osSemaphoreAttr_t MCSemEnd_attributes = {
  .name = "MCSemEnd"
};



/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* Functions that process MC data */
DrivingDirection direction(uint8_t data);
uint32_t error_code(uint16_t data);
uint32_t process_bits(uint16_t data);

/* Function prototypes for threads */
void MC_ReceiveStartThread(void *argument);
void MC_ReceiveEndThread(void *argument);
void MC_SendStartThread(void *argument);
void MC_SendEndThread(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

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

  /* USER CODE BEGIN RTOS_SEMAPHORES */

  MCSemStartHandle = osSemaphoreNew(1, 0, &MCSemStart_attributes);
  MCSemEndHandle = osSemaphoreNew(1, 0, &MCSemEnd_attributes);
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  start_frame_q = osMessageQueueNew(1, sizeof(CAN_Frame_t), NULL);
  end_frame_q = osMessageQueueNew(1, sizeof(CAN_Frame_t), NULL);

  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */

  /* USER CODE BEGIN RTOS_THREADS */

  MC_ReceiveStartTaskHandle = osThreadNew(MC_ReceiveStartThread, NULL, MC_ReceiveStartTaskAttr);
  MC_ReceiveEndTaskHandle = osThreadNew(MC_ReceiveEndThread, NULL, MC_ReceiveEndTaskAttr);
  MC_SendStartTaskHandle = osThreadNew(MC_SendStartThread, NULL, MC_SendStartTaskAttr);
  MC_SendEndTaskHandle = osThreadNew(MC_SendEndThread, NULL, MC_SendEndTaskAttr);

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

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void MC_ReceiveStartThread(void *argument) {

	CAN_Frame_t start_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER_K1); //Initialize CAN_Frame to send to MC to receive first data frame

	start_frame.id_type = CAN_ID_EXT;
	start_frame.data_length = 0;
	start_frame.rtr = 1; //Configure the request transmission bit to high

	uint8_t received = 0;

	for(;;){


		// Checks if mailbox is available for transmission and sends start_frame to the MC (requesting data bacK)
	    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
			CAN_send_frame(start_frame);
	    }

		if(HAL_CAN_GetRxFifoFillLevel(&hcan3, CAN_RX_FIFO0)){
			start_frame = CAN_get_frame(&hcan3, CAN_RX_FIFO0);
			received = 1;
		}
		// Checks if message is available in RxFifo and receives that message from the MC


		if(received){
			osMessageQueuePut(start_frame_q, &start_frame, 0, osWaitForever);
			osSemaphoreRelease(MCSemStartHandle);
		}
		//Checks if data was filled and puts the data in the queue, releasing the semaphore to indicate data is available

	}
}

void MC_ReceiveEndThread(void *argument) {

	CAN_Frame_t end_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER_K2); //Initialize CAN_Frame to send to MC to receive second data frame

	end_frame.id_type = CAN_ID_EXT;
	end_frame.data_length = 0;
	end_frame.rtr = 1;

	uint8_t received = 0;

	for(;;){

	    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
			CAN_send_frame(end_frame);
		}

		if(HAL_CAN_GetRxFifoFillLevel(&hcan3, CAN_RX_FIFO0)){
			end_frame = CAN_get_frame(&hcan3, CAN_RX_FIFO0);
			received = 1;
		}


		if(received){
			osMessageQueuePut(end_frame_q, &end_frame, 0, osWaitForever);
			osSemaphoreRelease(MCSemEndHandle);

		}

	}
}



void MC_SendStartThread(void *argument){

	CAN_Frame_t start_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER_K1); //Initialize CAN_frame to send to MC
	CAN_Frame_t send_frame = CAN_frame_init(&hcan1, MOTOR_CONTROLLER_K1); //Initialize CAN_frame to send to RPI

	Motor_Controller_t mc_data = {0};

	for(;;){

		osSemaphoreAcquire(MCSemStartHandle, osWaitForever);

		osMessageQueueGet(start_frame_q, &start_frame, NULL, osWaitForever);

		//Process data from MC
		mc_data.direction = direction(CAN_get_segment(start_frame, DRIVING_DIRECTION_K));
		mc_data.speed = CAN_get_segment(start_frame, MOTOR_SPEED_K);
		mc_data.error_code = error_code(CAN_get_segment(start_frame, MOTOR_ERROR_CODE_K));

		//Create new CAN_frame with proccessed data
		CAN_set_segment(&send_frame, DRIVING_DIRECTION_K, mc_data.direction);
		CAN_set_segment(&send_frame, MOTOR_SPEED_K, mc_data.speed);
		CAN_set_segment(&send_frame, MOTOR_ERROR_CODE_K, mc_data.error_code);

	    //Send data to RPI
	    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)) {
	        CAN_send_frame(send_frame);
	    }


	    osDelay(1);


	}
}

void MC_SendEndThread(void *argument){

	CAN_Frame_t end_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER_K2);
	CAN_Frame_t send_frame = CAN_frame_init(&hcan1, MOTOR_CONTROLLER_K2);

	Motor_Controller_t mc_data = {0};

	for(;;){

		osSemaphoreAcquire(MCSemEndHandle, osWaitForever);

		osMessageQueueGet(end_frame_q, &end_frame, NULL, osWaitForever);


		mc_data.voltage = process_bits(CAN_get_segment(end_frame, BATTERY_VOLTAGE_K));
		mc_data.current = process_bits(CAN_get_segment(end_frame, BATTERY_CURRENT_K));
		mc_data.motor_temp = process_bits(CAN_get_segment(end_frame, MOTOR_TEMP_K));
		mc_data.controller_temp = process_bits(CAN_get_segment(end_frame, MOTOR_CONTROLLER_TEMP_K));


		CAN_set_segment(&send_frame, BATTERY_VOLTAGE_K, mc_data.voltage);
		CAN_set_segment(&send_frame, BATTERY_CURRENT_K, mc_data.current);
		CAN_set_segment(&send_frame, MOTOR_TEMP_K, mc_data.motor_temp);
		CAN_set_segment(&send_frame, MOTOR_CONTROLLER_TEMP_K, mc_data.controller_temp);

	    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)) {
	    	CAN_send_frame(send_frame);
	    }

	    osDelay(1);


	}
}

DrivingDirection direction(uint8_t data){

	//Process direction data from MC according to data sheet

	uint8_t data_check = (data & 0b00000011);

	if(data_check == 0b00){
		return NEUTRAL;
	}
	else if(data_check == 0b01){
		return FORWARD;
	}
	else if(data_check == 0b10){
		return REVERSE;
	}

	return NEUTRAL;
}

uint32_t error_code(uint16_t data){
	return 0;
}

uint32_t process_bits(uint16_t data){
	return data/10;
}




/* USER CODE END Application */

