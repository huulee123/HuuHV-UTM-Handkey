/*
 * ADS1115.h
 *
 *  Created on: 26/12/2023
 *      Author: HUU
 */

#ifndef SRC_ADS1115_H_
#define SRC_ADS1115_H_

#include "stm32f1xx_hal.h"

#define ADS1115_ADDRESS 0x48
extern I2C_HandleTypeDef hi2c1;

#define ADC_CH_AO	1 // set channel 1 : AI2 GND
#define ADC_CH_VR	0 // set channel 0 : AI0 GND

void ADS1115_Init(uint8_t ch)
{
	// <0>: 0x01 CONFIG register address
	// <1>:
	// 		0x84 	MSB CH0 Continous
	//		0xB4	MSB CH1 Continous
	// 
	// 0x83 LSB (0x83 128Hz; 0xA3 250Hz; 0xC3 475Hz; 0xE3 860Hz)

	if (ch == 0)
	{
		uint8_t data[3] = { 0x01, 0x84, 0x83 };
		HAL_I2C_Master_Transmit(&hi2c1, ADS1115_ADDRESS << 1, data, 3, 100);
	}
	else
	{
		uint8_t data[3] = { 0x01, 0xB4, 0x83 };
		HAL_I2C_Master_Transmit(&hi2c1, ADS1115_ADDRESS << 1, data, 3, 100);
	}
}

int16_t ADS1115_ReadData()
{
	uint8_t data[2];
	HAL_I2C_Master_Transmit(&hi2c1, ADS1115_ADDRESS << 1, 0x00, 1, 100); // write to Data address 0x00
	//HAL_Delay(20);
	HAL_I2C_Master_Receive(&hi2c1, ADS1115_ADDRESS << 1, data, 2, 100);
	return (int16_t)(data[0] << 8 | data[1]);
}

#endif /* SRC_ADS1115_H_ */
