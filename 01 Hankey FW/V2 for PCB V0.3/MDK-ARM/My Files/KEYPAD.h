/*
 * KEYPAD.h
 *
 *  Created on: 13.11.2024
 *      Author: Trong Huu
 */


#ifndef KEYPAD_H_
#define KEYPAD_H_

#include "main.h"

typedef enum
{
	KEY_SHIFT_SET	= 4,
	KEY_ADD 		= 8,
	KEY_SUB			= 16,
	KEY_FUP			= 2,
	KEY_FDOWN		= 1,
	KEY_UP			= 32,
	KEY_STOP		= 128,
	KEY_DOWN		= 64,
	KEY_F1F2		= 12,
	KEY_F1F3		= 20,
	KEY_F2F3		= 24,
	KEY_FSTOP		= 200, // custom key

	KEY_NONE 		= 0,
} KEY_t;


uint8_t KEY_Get(void);

#endif /* KEYPAD_H_ */
