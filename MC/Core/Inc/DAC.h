/*
 * dac.h
 *
 *  Created on: Feb 4, 2024
 *      Author: Mostafa Hussein
 */

#ifndef DAC_H
#define DAC_H

#include "i2c.h"
#include "math.h"

#define DAC_I2C_WRITE_ADDRESS 0x62U

#define GENERAL_CALL_ADDRESS 0x00U
#define GENERAL_CALL_RESET 0x06U
#define GENERAL_CALL_WAKEUP 0x09U
#define HIGH_SPEED_MODE 0x0FU

#define MAX_CODE 4095

#define DAC_OFFSET 0

typedef struct {
	I2C_HandleTypeDef* hi2c;
	float vref;
	uint8_t dac_address;
	uint8_t buffer[2];
	uint8_t high_speed_mode;
} DAC_t;

// Sends a reset command to ensure EEPROM data is available to output
DAC_t DAC_init(I2C_HandleTypeDef* hi2c);

// Writes to DAC registers only
void DAC_write(DAC_t* dacInstance, float voltage);


// Calculates the required input code to reach a desired function
// Equation 5-1 (Ideal Transfer fnc)
// Vout_predicted = (Vref * Input Code) / 2^12
// Actual Vout = Vout_predicted + offset
//
// Ideally sticking to range of codes 100 to 4000 to avoid gain errors
static uint16_t calculate_code(DAC_t* dacInstance, float voltage);

// Sends reset call to the DAC
static void reset_dac(DAC_t* dacInstance);

// Sends wake up call to the DAC
static void wake_up_dac(DAC_t* dacInstance);

// Sends a command (uses fast mode by default)
static void send_command(DAC_t* dacInstance);


#endif /* DAC_H */
