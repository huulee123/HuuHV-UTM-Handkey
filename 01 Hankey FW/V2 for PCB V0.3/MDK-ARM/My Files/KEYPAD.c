/*
 * KEYPAD.c
 *
 *  Created on: 13.11.2024
 *      Author: Trong Huu
 */

#include "KEYPAD.h"
#include "HWIO.h"

uint8_t KEY_Get(void)
{
	uint8_t num = 0;

	PIN_RESET(PIN_KEY_LOAD); // LOAD = 0
	__NOP(); __NOP(); // __NOP(); __NOP();
	PIN_SET(PIN_KEY_LOAD);	 // LOAD = 1

	// Read bit7
	if (PIN_READ(PIN_KEY_DATA))
		num |= (1 << 7);

	// generate pulse in CLK to read
	for (int8_t i = 6; i >= 0; i--)
	{
		PIN_SET(PIN_KEY_CLK);
		PIN_RESET(PIN_KEY_CLK);

		if (KEY_DATA_GPIO_Port->IDR & KEY_DATA_Pin)
			num |= (1 << i);
	}

	return num;
}

