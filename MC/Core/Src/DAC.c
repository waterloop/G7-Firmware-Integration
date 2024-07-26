/*
 * dac.c
 *
 *  Created on: Feb 6, 2024
 *      Author: Mostafa Hussein
 */

#include "dac.h"

DAC_t DAC_init(I2C_HandleTypeDef* hi2c)
{
	DAC_t dacInstance;
	dacInstance.vref = 5;
	dacInstance.dac_address = DAC_I2C_WRITE_ADDRESS;
	dacInstance.high_speed_mode = 0;
	dacInstance.hi2c = hi2c;
	reset_dac(&dacInstance);

	return dacInstance;
}

void DAC_write(DAC_t* dacInstance, float voltage)
{
	uint16_t tempVal = calculate_code(dacInstance, voltage);
	dacInstance->buffer[0] = tempVal >> 8 & 0xFF;
	dacInstance->buffer[1] = tempVal & 0xFF;
	send_command(dacInstance);
}

static uint16_t calculate_code(DAC_t* dacInstance, float voltage)
{
	voltage = fabs(voltage - (0.01 * (voltage * 2)));
	return (uint16_t)(voltage * MAX_CODE / dacInstance->vref - DAC_OFFSET);
}

static void reset_dac(DAC_t* dacInstance)
{
	uint8_t resetTxBuffer = GENERAL_CALL_RESET;
	HAL_I2C_Master_Transmit(
				dacInstance->hi2c,
				GENERAL_CALL_ADDRESS,
				&resetTxBuffer,
				1,
				50
		);
}

static void wake_up_dac(DAC_t* dacInstance)
{
	uint8_t wakeupTxBuffer = GENERAL_CALL_WAKEUP;
	HAL_I2C_Master_Transmit(
				dacInstance->hi2c,
				GENERAL_CALL_ADDRESS,
				&wakeupTxBuffer,
				1,
				50
		);
}

static void send_command(DAC_t* dacInstance)
{
	uint16_t address = dacInstance->dac_address;
	address |= (HIGH_SPEED_MODE * dacInstance->high_speed_mode) << 8;
	HAL_I2C_Master_Transmit(
			dacInstance->hi2c,
			address<<1,
			dacInstance->buffer,
			2,
			50
	);
}


