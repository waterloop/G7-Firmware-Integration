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
#include "DAC.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {
	CAN_HandleTypeDef* hcan;
	DAC_t throttle;
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

typedef enum {
	MC_IDLE,
	MC_START,
	MC_STOP,
	MC_THROTTLE,
	MC_DIRECTION
} MC_Commands;

typedef struct {
	GPIO_TypeDef* port;
	uint16_t pin;
} GPIO_Pin_t;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define DEFAULT_THROTTLE 100

#define MAX_SEM_COUNT 1


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
const GPIO_Pin_t FW = {.port = GPIOD, .pin = GPIO_PIN_11};
const GPIO_Pin_t REV = {.port = GPIOD, .pin = GPIO_PIN_12};
const GPIO_Pin_t BRK = {.port = GPIOD, .pin = GPIO_PIN_13};


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
		.priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t MC_SendEndTaskHandle;
const osThreadAttr_t MC_SendEndTaskAttr = {
		.name = "MC_SendEndThread",
		.stack_size = 128*4,
		.priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t MC_CommandControlTaskHandle;
const osThreadAttr_t MC_CommandControlTaskAttr = {
		.name = "MC_CommandControlThread",
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


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* Functions that process MC data */
DrivingDirection bits_to_direction(uint8_t data);
uint32_t error_code(uint16_t data);
uint32_t process_bits(uint16_t data);

/* Function prototypes for threads */
void MC_ReceiveStartThread(void *argument);
void MC_ReceiveEndThread(void *argument);
void MC_SendStartThread(void *argument);
void MC_SendEndThread(void *argument);
void MC_CommandControlThread(void *argument);

/* Function prototypes for motor control*/
Motor_Controller_t MC_init(CAN_HandleTypeDef* can_handler, I2C_HandleTypeDef* i2c_handler);
void MC_stop(Motor_Controller_t* self);
void MC_drive(Motor_Controller_t* self);
void MC_change_direction(void);
void MC_set_throttle(Motor_Controller_t* self, float value);
void MC_get_data(Motor_Controller_t* self);
void MC_execute_command(CAN_Frame_t frame);

float get_voltage(float throttle);



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

  MC_ReceiveStartTaskHandle = osThreadNew(MC_ReceiveStartThread, NULL, &MC_ReceiveStartTaskAttr);
  MC_ReceiveEndTaskHandle = osThreadNew(MC_ReceiveEndThread, NULL, &MC_ReceiveEndTaskAttr);
  MC_SendStartTaskHandle = osThreadNew(MC_SendStartThread, NULL, &MC_SendStartTaskAttr);
  MC_SendEndTaskHandle = osThreadNew(MC_SendEndThread, NULL, &MC_SendEndTaskAttr);
  MC_CommandControlTaskHandle = osThreadNew(MC_CommandControlThread, NULL, &MC_CommandControlTaskAttr);

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

	/*Setting ID to MOTOR_CONTROLLER_K1 ensures that we receive the first data frame from the MC*/

	CAN_Frame_t start_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER_K1); //Initialize CAN_Frame to send to MC

	start_frame.id_type = CAN_ID_EXT;
	start_frame.data_length = 0;
	start_frame.rtr = 1; //Configure the request transmission bit to high

	uint8_t received = 0;

	for(;;){

		//Checks if mailbox is available for transmission and sends start_frame to the MC (requesting data bacK)
	    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
			CAN_send_frame(start_frame);
	    }

	    //Checks if message is available in RxFifo and receives that message from the MC
		if(HAL_CAN_GetRxFifoFillLevel(&hcan3, CAN_RX_FIFO0)){
			start_frame = CAN_get_frame(&hcan3, CAN_RX_FIFO0);
			received = 1;
		}

		//Checks if data was filled and puts the data in the queue, releasing the semaphore to indicate data is available
		if(received){
			osMessageQueuePut(start_frame_q, &start_frame, 0, osWaitForever);
			osSemaphoreRelease(MCSemStartHandle);
		}


	}
}

void MC_ReceiveEndThread(void *argument) {

	/*Setting ID to MOTOR_CONTROLLER_K2 ensures that we receive the first data frame from the MC*/
	CAN_Frame_t end_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER_K2); //Initialize CAN_Frame to send to MC

	end_frame.id_type = CAN_ID_EXT;
	end_frame.data_length = 0;
	end_frame.rtr = 1; //Configure the request transmission bit to high

	uint8_t received = 0;

	for(;;){

		//Checks if mailbox is available for transmission and sends start_frame to the MC (requesting data bacK)
	    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
			CAN_send_frame(end_frame);
		}

	    //Checks if message is available in RxFifo and receives that message from the MC
		if(HAL_CAN_GetRxFifoFillLevel(&hcan3, CAN_RX_FIFO0)){
			end_frame = CAN_get_frame(&hcan3, CAN_RX_FIFO0);
			received = 1;
		}

		//Checks if data was filled and puts the data in the queue, releasing the semaphore to indicate data is available
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
		mc_data.direction = bits_to_direction(CAN_get_segment(start_frame, DRIVING_DIRECTION_K));
		mc_data.speed = CAN_get_segment(start_frame, MOTOR_SPEED_K);
		mc_data.error_code = error_code(CAN_get_segment(start_frame, MOTOR_ERROR_CODE_K));

		//Create new CAN_frame with processed data
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

	CAN_Frame_t end_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER_K2); //Initialize CAN_frame to send to MC
	CAN_Frame_t send_frame = CAN_frame_init(&hcan1, MOTOR_CONTROLLER_K2); //Initialize CAN_frame to send to RPI

	Motor_Controller_t mc_data = {0};

	for(;;){

		osSemaphoreAcquire(MCSemEndHandle, osWaitForever);

		osMessageQueueGet(end_frame_q, &end_frame, NULL, osWaitForever);

		//Process data from MC
		mc_data.voltage = process_bits(CAN_get_segment(end_frame, BATTERY_VOLTAGE_K));
		mc_data.current = process_bits(CAN_get_segment(end_frame, BATTERY_CURRENT_K));
		mc_data.motor_temp = process_bits(CAN_get_segment(end_frame, MOTOR_TEMP_K));
		mc_data.controller_temp = process_bits(CAN_get_segment(end_frame, MOTOR_CONTROLLER_TEMP_K));

		//Create new CAN_frame with processed data
		CAN_set_segment(&send_frame, BATTERY_VOLTAGE_K, mc_data.voltage);
		CAN_set_segment(&send_frame, BATTERY_CURRENT_K, mc_data.current);
		CAN_set_segment(&send_frame, MOTOR_TEMP_K, mc_data.motor_temp);
		CAN_set_segment(&send_frame, MOTOR_CONTROLLER_TEMP_K, mc_data.controller_temp);

		//Send data to RPI
	    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)) {
	    	CAN_send_frame(send_frame);
	    }

	    osDelay(1);


	}
}

void MC_CommandControlThread(void *argument){
	CAN_Frame_t command_frame = CAN_frame_init(&hcan3, MOTOR_CONTROLLER); //initialize frame to receive command from RPI (psuedo frame, RPI code doesn't exist yet)
	Motor_Controller_t kelly_mc =  MC_init(&hcan3, &hi2c2);; //Initialize motor controller with used CAN and I2C handles

	for(;;){

		//Checks if CAN message is available from RPI
		if(HAL_CAN_GetRxFifoFillLevel(&hcan3, CAN_RX_FIFO1)){
			command_frame = CAN_get_frame(&hcan3, CAN_RX_FIFO1);
		}

		uint8_t command = CAN_get_segment(command_frame, RPI_COMMAND_CODE); //Get "command" from CAN data frame

		if(command == MC_START){
			MC_drive(&kelly_mc);
		}
		else if(command == MC_STOP){
			MC_stop(&kelly_mc);
		}
		else if(command == MC_THROTTLE){
			uint8_t throttle = CAN_get_segment(command_frame, RPI_COMMAND_DATA); //Get throttle percent "data" from CAN data frame
			MC_set_throttle(&kelly_mc, (float)throttle);
		}
		else if(command == MC_DIRECTION){
			MC_change_direction();
		}

	}

}

DrivingDirection bits_to_direction(uint8_t data){

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
	//Processes data from MC according to data sheet (e.g 0.1 C / bit)
	return data/10;
}

Motor_Controller_t MC_init(CAN_HandleTypeDef* handler, I2C_HandleTypeDef* i2c_handler) {

	//Initializes Motor_Controller_t struct with base values and proper CAN and I2C handles
	Motor_Controller_t ret = {
		.hcan = handler,
		.direction = 0,
		.speed = 0,
		.error_code = 0,
		.voltage = 0,
		.current = 0,
		.motor_temp = 0,
		.controller_temp = 0,
		.throttle = DAC_init(i2c_handler)
	};

	return ret;
}

void MC_stop(Motor_Controller_t* self) {
	DAC_write(&self -> throttle, 0); //Sets throttle to 0
	HAL_GPIO_WritePin(BRK.port, BRK.pin, GPIO_PIN_SET); //Sets brake pin
}

void MC_drive(Motor_Controller_t* self) {
	HAL_GPIO_WritePin(BRK.port, BRK.pin, GPIO_PIN_RESET); //Resets brake pin (0)
	HAL_GPIO_WritePin(REV.port, REV.pin, GPIO_PIN_RESET); //Resets reverse pin
	HAL_GPIO_WritePin(FW.port, FW.pin, GPIO_PIN_SET); //Sets forward pin
	DAC_write(&self -> throttle, get_voltage(DEFAULT_THROTTLE)); //Sets throttle to default (100%)
}

void MC_change_direction() {
	if (HAL_GPIO_ReadPin(FW.port, FW.pin)) { //Sets direction to reverse if direction is forward
		HAL_GPIO_WritePin(FW.port, FW.pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(REV.port, REV.pin, GPIO_PIN_SET);
	} else { //Sets direction to forward if direction is reverse
		HAL_GPIO_WritePin(REV.port, REV.pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(FW.port, FW.pin, GPIO_PIN_SET);
	}
}

void MC_set_throttle(Motor_Controller_t* self, float value) {
	DAC_write(&self -> throttle, get_voltage(value)); //Updates MC throttle with passed in value
}

float get_voltage(float throttle) {
  return throttle/100.0 * 5;
}




/* USER CODE END Application */

