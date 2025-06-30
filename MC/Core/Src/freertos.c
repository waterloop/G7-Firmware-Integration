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
#include "tim.h"
#include "can.h"
#include "can_driver.h"
#include "DAC.h"
#include "config.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticSemaphore_t osStaticMutexDef_t;
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

typedef struct {
	MC_Commands command;
	DrivingDirection direction;
	uint32_t throttle;
} Parsed_Command_t;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

volatile Parsed_Command_t mc_command;
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

osThreadId_t MC_ReceiveCommandTaskHandle;
const osThreadAttr_t MC_ReceiveCommandTaskAttr = {
		.name = "MC_ReceiveCommandThread",
		.stack_size = 128 * 4,
		.priority = (osPriority_t)  osPriorityHigh1,
};

osThreadId_t MC_CommandControlTaskHandle;
const osThreadAttr_t MC_CommandControlTaskAttr = {
		.name = "MC_CommandControlThread",
		.stack_size = 128*4,
		.priority = (osPriority_t) osPriorityHigh,
};

/* Queue definitions for CAN frames */


/* Definitions for start and end CAN frame semaphores */
osSemaphoreId_t MCSemReceiveHandle;
const osSemaphoreAttr_t MCSemReceive_attributes = {
  .name = "MCSemReceive"
};

osSemaphoreId_t MCSemParsedHandle;
const osSemaphoreAttr_t MCSemParsed_attributes = {
  .name = "MCSemParsed"
};



/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CommandMutex */
osMutexId_t CommandMutexHandle;
osStaticMutexDef_t myMutex01ControlBlock;
const osMutexAttr_t CommandMutex_attributes = {
  .name = "CommandMutex",
  .cb_mem = &myMutex01ControlBlock,
  .cb_size = sizeof(myMutex01ControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* Functions that process MC data */
DrivingDirection bits_to_direction(uint8_t data);
uint32_t error_code(uint16_t data);
uint32_t process_bits(uint16_t data);

/* Function prototypes for threads */
void MC_ReceiveCommandThread(void *argument);
void MC_CommandControlThread(void *argument);

/* Function prototypes for motor control*/
Motor_Controller_t MC_init(CAN_HandleTypeDef* can_handler, I2C_HandleTypeDef* i2c_handler);
void MC_stop(Motor_Controller_t* self);
void MC_drive(Motor_Controller_t* self);
void MC_change_direction(Motor_Controller_t* self);
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
  /* Create the mutex(es) */
  /* creation of CommandMutex */
  CommandMutexHandle = osMutexNew(&CommandMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */

  MCSemReceiveHandle = osSemaphoreNew(1, 0, &MCSemReceive_attributes);
  MCSemParsedHandle = osSemaphoreNew(1, 0, &MCSemParsed_attributes);
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */

  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

  MC_ReceiveCommandTaskHandle = osThreadNew(MC_ReceiveCommandThread, NULL, &MC_ReceiveCommandTaskAttr);
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

void MC_ReceiveCommandThread(void *argument) {

	CAN_Frame_t command_frame = CAN_frame_init(&hcan1, MOTOR_CONTROLLER);
	for(;;){

		osSemaphoreAcquire(MCSemReceiveHandle, osWaitForever);
		command_frame = CAN_get_frame(&hcan1, CAN_RX_FIFO0);
		Parsed_Command_t received_command;

		switch(command_frame.data[0]){
			case MC_IDLE:
				received_command.command = MC_IDLE;
				received_command.direction = NEUTRAL;
				received_command.throttle = 0;
				break;

			case MC_START:
				received_command.command = MC_START;
				received_command.direction = FORWARD;
				received_command.throttle = command_frame.data[1];
				break;

			case MC_STOP:
				received_command.command = MC_STOP;
				received_command.direction = NEUTRAL;
				received_command.throttle = 0;
				break;

			case MC_THROTTLE:
				received_command.command = MC_THROTTLE;
				received_command.throttle = command_frame.data[1];
				break;

			case MC_DIRECTION:
				received_command.command = MC_DIRECTION;
				break;

			default:
				break;
		}

		osMutexAcquire(CommandMutexHandle, osWaitForever);
		mc_command = received_command;
		osMutexRelease(CommandMutexHandle);

		osSemaphoreRelease(MCSemParsedHandle);
	}
}

void MC_CommandControlThread(void *argument){
	Parsed_Command_t parsed_command;
	Motor_Controller_t kelly_mc =  MC_init(&hcan1, &hi2c2);; //Initialize motor controller with used CAN and I2C handles

	for(;;){

		osSemaphoreAcquire(MCSemParsedHandle, osWaitForever);

		osMutexAcquire(CommandMutexHandle, osWaitForever);
		parsed_command = mc_command;
		osMutexRelease(CommandMutexHandle);

		switch(parsed_command.command){
			case MC_IDLE:
				break;

			case MC_START:
				kelly_mc.direction = parsed_command.direction;
				kelly_mc.throttle.throttle = parsed_command.throttle;
				kelly_mc.speed = parsed_command.throttle;
				MC_drive(&kelly_mc);
				break;

			case MC_STOP:
				kelly_mc.direction = parsed_command.direction;
				kelly_mc.throttle.throttle = parsed_command.throttle;
				MC_stop(&kelly_mc);
				break;

			case MC_THROTTLE:
				kelly_mc.direction = parsed_command.direction;
				kelly_mc.throttle.throttle = parsed_command.throttle;
				kelly_mc.speed = parsed_command.throttle;
				MC_set_throttle(&kelly_mc, (float)parsed_command.throttle);
				break;

			case MC_DIRECTION:
				kelly_mc.direction = parsed_command.direction;
				MC_change_direction(&kelly_mc);
				break;

			default:
				break;
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

void MC_set_motor_speed(uint32_t freq, uint8_t direction){

    uint32_t timer_clock = 16000000; // 16 MHz timer clock
    uint32_t prescaler = 0; // assuming prescaler 0 for max resolution

    uint32_t arr = (timer_clock / ((prescaler + 1) * freq)) - 1;
    uint32_t pulse = (arr + 1)/2;
    uint32_t phase_offset = pulse/2;

    __HAL_TIM_DISABLE(&htim1);    // Stop timer
    __HAL_TIM_DISABLE(&htim2);    // Stop timer
    __HAL_TIM_SET_AUTORELOAD(&htim1, arr);  // Update ARR
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);  // Update ARR
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);

    if(direction == 1){
        __HAL_TIM_SET_COUNTER(&htim2, 3 * phase_offset); //Start at 270 degrees = lead timer1 by 90 degrees
    }
    else if(direction == 0){
    	__HAL_TIM_SET_COUNTER(&htim2, phase_offset); //Start at 90 degrees = lags timer1 by 90 degrees
    }

    __HAL_TIM_ENABLE(&htim1);     // Restart timer
    __HAL_TIM_ENABLE(&htim2);     // Restart timer
}

void MC_stop(Motor_Controller_t* self) {
	HAL_GPIO_WritePin(BRK.port, BRK.pin, GPIO_PIN_SET); //Sets brake pin
	HAL_GPIO_WritePin(REV.port, REV.pin, GPIO_PIN_RESET); //Resets reverse pin
	HAL_GPIO_WritePin(FW.port, FW.pin, GPIO_PIN_RESET); //Resets forward pin

	uint32_t step_delay = 50; // 50ms steps
	uint32_t total_steps = 1000 / step_delay;

	for (uint32_t step = 0; step < total_steps; step++) {
	   float progress = (float)step / (float)total_steps;
	   float new_speed = self->speed * (1.0f - progress);

	   MC_set_motor_speed((new_speed/100) * 10000, self->direction);
	   osDelay(pdMS_TO_TICKS(step_delay));
	}
}

void MC_drive(Motor_Controller_t* self) {
	HAL_GPIO_WritePin(BRK.port, BRK.pin, GPIO_PIN_RESET); //Resets brake pin (0)
	HAL_GPIO_WritePin(REV.port, REV.pin, GPIO_PIN_RESET); //Resets reverse pin
	HAL_GPIO_WritePin(FW.port, FW.pin, GPIO_PIN_SET); //Sets forward pin
    MC_set_motor_speed((self->speed/100) * 10000, 1); //10 khz frequency and forward

	//DAC_write(&self -> throttle, get_voltage(self->throttle.throttle)); //Sets throttle to default (100%)
}

void MC_change_direction(Motor_Controller_t* self) {
	if (HAL_GPIO_ReadPin(FW.port, FW.pin)) { //Sets direction to reverse if direction is forward
		HAL_GPIO_WritePin(FW.port, FW.pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(REV.port, REV.pin, GPIO_PIN_SET);
		MC_set_motor_speed((self->speed/100) * 10000, 0); //keep same speed, change direction to reverse
	} else if(HAL_GPIO_ReadPin(REV.port, REV.pin)) { //Sets direction to forward if direction is reverse
		HAL_GPIO_WritePin(REV.port, REV.pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(FW.port, FW.pin, GPIO_PIN_SET);
		MC_set_motor_speed((self->speed/100) * 10000, 1);
	}
}

void MC_set_throttle(Motor_Controller_t* self, float value) {
	 MC_set_motor_speed((value/100) * 10000, self->direction);
}

float get_voltage(float throttle) {
  return throttle / 20.0;
}



void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(hcan->Instance == CAN1){
		osSemaphoreRelease(MCSemReceiveHandle);
	}
}


/* USER CODE END Application */

