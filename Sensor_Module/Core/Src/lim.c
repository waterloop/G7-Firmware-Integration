/*
 * lim.c
 *
 *  Created on: May 9, 2024
 *      Author: mahir
 */

#include "lim.h"
#include "adc.h"
#include <math.h>

void get_lim_data(uint8_t lim_temps[NUM_THERM_TOTAL]) {
	for (uint8_t i = 0; i < NUM_LIMS; i++) {
		  		  lim_temps[i] = 0;
	}

	for (uint8_t i = 0; i < NUM_THERM_TOTAL; i++) {
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, i & 0x1);
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, (i >> 1) & 0x1);
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_2, (i >> 2) & 0x1);

		HAL_ADC_Start(&hadc3);
		HAL_ADC_PollForConversion(&hadc3, 100);
		uint16_t adc_data = HAL_ADC_GetValue(&hadc3);

		lim_temps[i] += get_temp(adc_data);
	}
}

uint8_t get_temp(uint16_t adc_value) {
		float voltage_in = adc_value * (MAX_VOLTAGE / MAX_ADC_COUNT);
		float thermistor_resistance = (voltage_in * R10K) / (VOLTAGE_SUPPLY - voltage_in);

		uint8_t temp_steinhart = -ABSOLUTE_ZERO + (NOMINAL_TEMPERATURE * B_COEFFICIENT)/(NOMINAL_TEMPERATURE
				                 * log(thermistor_resistance / R10K) + B_COEFFICIENT);

		return temp_steinhart;
}
