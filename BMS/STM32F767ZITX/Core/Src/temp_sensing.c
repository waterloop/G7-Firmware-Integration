#include "temp_sensing.h"
#include <math.h>
#include <stdio.h>


//Function: Adds temperature sample to array stored in memory and updates final average temperature table
//Inputs:   temp_average(old array of average temperatures for each MUX over n samples),
//          temp_snapshot(array containing 1 sample of MUX temperature data),
//          temp_data(array stored in memory containing past n temperature samples)
//Outputs:  average_mux_temp(updated array of average temperatures for each MUX over n samples), temp_data(modified temperature array)
void enqueue(uint32_t *temp_average, uint32_t temp_snapshot[NUM_MUX], uint32_t *temp_data[NUM_MUX]) {
	rear = (rear + 1) % NUM_SAMPLES;
	for (uint8_t i = 0U; i < NUM_MUX; ++i){
		temp_average[i] -= temp_data[rear][i] / NUM_SAMPLES;
		temp_data[rear][i] = temp_snapshot[i];
		temp_average[i] += temp_data[rear][i] / NUM_SAMPLES;
	}
	return;
}

//Function: Calculates thermistor value in resistance and converts to degrees C using the Steinhart-Hart equation
//Inputs:   adc_data(reading from the ADC)
//Outputs:  tempSteinhart(temperature in degrees C)
uint32_t calculateTemperature (uint32_t adc_data) {
	//Convert the ADC value being read into a resistance.
	//R = 8250 / (4096 / (ADC*GainTranslation) - 1)
	uint32_t voltage_out = adc_data * VOLTAGE_SUPPLY / 4095U;
	uint32_t thermistor_resistance = voltage_out * R10K / (VOLTAGE_SUPPLY - voltage_out);

	//Calculates Temperature from Resistance of thermistor using the Simplified B parameter Steinhart Equation.
	//1/Temp = 1/NominalTemp + (1/B)*1n(Thermistor Resistance/NominalResistance)
	uint32_t temp_steinhart = -ABSOLUTE_ZERO + (1.0/((1.0/ (NOMINAL_TEMPERATURE + ABSOLUTE_ZERO)) + (log(thermistor_resistance / NOMINAL_RESISTANCE) / B_COEFFICIENT)));
	return temp_steinhart;
}

//Function: Measures average thermistor temperature of each MUX
//Inputs:   adc_data(array containing measured values from ADCs),
//          temp_data(array stored in memory containing past n temperature samples),
//          temp_average(array containing average multiplexor temperature)
//Outputs:  temp_data(modified temperature array)
void measureTempADC(uint32_t *temp_average, uint32_t *adc_data, uint32_t *temp_data[NUM_MUX])
{
	uint32_t temp_snapshot[NUM_MUX] = {0};
	for(uint8_t i = 0U; i < NUM_THERM_PER_MUX; ++i){
		//Increment orientation of MUX control lines.
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, i & 0x1);
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, i >> 1 & 0x1);
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_2, i >> 2 & 0x1);

		//Record average temperature of each MUX.
		for(uint8_t i = 0U; i < NUM_MUX; ++i){
			temp_snapshot[i] += calculateTemperature(adc_data[i]) / NUM_THERM_PER_MUX;
		}
	}
	enqueue(temp_average, temp_snapshot, temp_data);
	return;
}

//Function: Powers fan using
//Inputs:   adc_data(array containing measured values from ADCs),
//          temp_data(array stored in memory containing past n temperature samples),
//          temp_average(array containing average multiplexor temperature)
//Outputs:  temp_data(modified temperature array)
void powerFan()
{
	return;
}
