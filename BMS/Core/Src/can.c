/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
#include "can.h"

/* USER CODE BEGIN 0 */

#include "config.h"
/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;

/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 1;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void CAN_Filter_config(void){

	CAN_FilterTypeDef can_filter;

		can_filter.FilterActivation = ENABLE;
		can_filter.FilterBank = 0;
		can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
		can_filter.FilterIdHigh = 0x0;
		can_filter.FilterIdLow = 0x0;
		can_filter.FilterMaskIdHigh = 0x00;
		can_filter.FilterMaskIdLow = 0x0;
		can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
		can_filter.FilterScale = CAN_FILTERSCALE_32BIT;

		if ( HAL_CAN_ConfigFilter(&hcan1, &can_filter) != HAL_OK){
			Error_Handler();
		}

}

void Can_Health(void){

	CAN_RxHeaderTypeDef canRx;

	uint8_t rcvd_health[4] = {0};


		while( ! HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0));

		if(HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &canRx, rcvd_health) != HAL_OK){
			Error_Handler();
		}

		if(rcvd_health[0] == 0x01 && rcvd_health[1] == 0x02 && rcvd_health[3] == 0x03 && rcvd_health[4] == 0x04){


			CAN_Frame_t tx_frame = CAN_frame_init(&hcan1, RPI_ID);

			CAN_set_segment(&tx_frame, RPI_SEG_1 , rcvd_health[0]);
			CAN_set_segment(&tx_frame, RPI_SEG_2 , rcvd_health[1]);
			CAN_set_segment(&tx_frame, RPI_SEG_3 , rcvd_health[2]);
			CAN_set_segment(&tx_frame, RPI_SEG_4 , rcvd_health[3]);


		}



}

void CAN_TransmitLED(void){

		CAN_TxHeaderTypeDef canh ={0};


		uint8_t our_msg = {0};
		uint32_t mailbox;
		canh.DLC = 1;
		canh.StdId = 0x65a;
		canh.IDE = CAN_ID_STD;
		canh.RTR = CAN_RTR_DATA;


		our_msg = 8;

		if(HAL_CAN_AddTxMessage(&hcan1, &canh, &our_msg, &mailbox)!= HAL_OK){
			Error_handler();


}
}

/* USER CODE END 1 */
