#pragma once

#include <stdint.h>
#include "adc.h"

#define NUM_MUX 6U //Note: the number of multiplexors must also be changed in the ADC3 configuration
#define NUM_THERM_PER_MUX 8U

#define NUM_SAMPLES 50U
#define SAMPLE_DELAY 10U

#define R10K 10000U
#define VOLTAGE_SUPPLY 2.5U

//Steinhart-Hart constants
#define BIAS_RESISTANCE 8250U
#define MAX_ADC_COUNT 4096U
#define GAIN_TRANSLATE 1U //fill in with correct value
#define ABSOLUTE_ZERO 273.15 //fill in with correct value
#define NOMINAL_TEMPERATURE 1U //fill in with correct value
#define NOMINAL_RESISTANCE 1U //fill in with correct value
#define B_COEFFICIENT 1U //fill in with correct value

static int8_t rear = -1;
//static uint32_t temp_data[NUM_SAMPLES][NUM_MUX];

void enqueue(uint32_t *temp_average, uint32_t temp_snapshot[NUM_MUX], uint32_t *temp_data[NUM_MUX]);
void measureTempADC(uint32_t *temp_average, uint32_t *adc_data, uint32_t *temp_data[NUM_MUX]);
uint32_t calculateTemperature (uint32_t adc_data);
void powerFan();

