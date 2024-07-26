/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "can.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can_driver.h"
#include "config.h"
#include "mpu6050.h"
#include "lim.h"

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_CAN3_Init();
  MX_I2C2_Init();
  MX_ADC3_Init();
  /* USER CODE BEGIN 2 */
  HAL_CAN_Start(&hcan3);
  MPU6050_Init(hi2c2);
  //configure filters
  uint8_t pressure = 0;
  int16_t x_accel = 0;
  int16_t y_accel = 0;
  int8_t x_gyro = 0;
  int8_t y_gyro = 0;
  int8_t z_gyro = 0;
  uint8_t lim_temps[NUM_THERM_TOTAL];
  uint8_t error_code_1 = 0;
  uint8_t error_code_2 = 0;

  CAN_Frame_t tx_frame = CAN_frame_init(&hcan3, SENSOR_BOARD_1);
  CAN_Frame_t imu_frame = CAN_frame_init(&hcan3, SENSOR_BOARD_2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  //poll pressure sensor
	  pressure = poll_pressure_sensor();

	  //poll IMU
	  MPU6050_Read_Accel(&x_accel, &y_accel);
	  MPU6050_Read_Gyro(&x_gyro, &y_gyro, &z_gyro);

	  //poll LIM thermistors
	  get_lim_data(lim_temps);

	  //Pack CAN messages
	  CAN_set_segment(&tx_frame, PRESSURE, pressure);
	  CAN_set_segment(&tx_frame, LIM_ONE_TEMP_ONE, lim_temps[0]);
	  CAN_set_segment(&tx_frame, LIM_ONE_TEMP_TWO, lim_temps[1]);
	  CAN_set_segment(&tx_frame, LIM_ONE_TEMP_THREE, lim_temps[2]);
	  CAN_set_segment(&tx_frame, LIM_TWO_TEMP_ONE, lim_temps[3]);
	  CAN_set_segment(&tx_frame, LIM_TWO_TEMP_TWO, lim_temps[4]);
	  CAN_set_segment(&tx_frame, LIM_TWO_TEMP_THREE, lim_temps[5]);
	  CAN_set_segment(&tx_frame, SENSORS_ERROR_CODE_1, error_code_1);

	  CAN_set_segment(&imu_frame, X_ACCEL, x_accel);
	  CAN_set_segment(&imu_frame, Y_ACCEL, y_accel);
	  CAN_set_segment(&imu_frame, X_GYRO, x_gyro);
	  CAN_set_segment(&imu_frame, Y_GYRO, y_gyro);
	  CAN_set_segment(&imu_frame, Z_GYRO, z_gyro);
	  CAN_set_segment(&imu_frame, SENSORS_ERROR_CODE_2, error_code_2);

	  //Send CAN messages
	  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
		  CAN_send_frame(tx_frame);
	  }
	  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan3)) {
		  CAN_send_frame(imu_frame);
	  }

	  HAL_Delay(500);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

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