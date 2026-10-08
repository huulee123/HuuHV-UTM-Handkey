/**
  ******************************************************************************
  * @Date           : 05.11.2025
  * @Author			: Trong Huu
  * @Brief          : Faster IO function
  ******************************************************************************
  * @UpdateLog


  * @Note


  ******************************************************************************
  */

#ifndef HWIO_H_
#define HWIO_H_

#include "main.h"

#define PIN_SET(name)    	((pinList[(name)].Port)->BSRR = (pinList[(name)].Pin))
#define PIN_RESET(name)  	((pinList[(name)].Port)->BSRR = ((uint32_t)(pinList[(name)].Pin) << 16))
#define PIN_READ(name)  	(((pinList[(name)].Port->IDR & pinList[(name)].Pin)) ? 1 : 0)

// HW pin define
typedef struct 
{
    GPIO_TypeDef *Port;
    uint16_t Pin;
} HW_PIN_t;

typedef enum
{
	PIN_SW1 = 0,
	PIN_SW2,
	PIN_I0,
	PIN_I1,
	PIN_I2,
	PIN_I3,
	PIN_I4,
	PIN_Q0,
	PIN_Q1,
	PIN_Q2,
	PIN_Q3,
	PIN_BIT0,
	PIN_BIT1,
	PIN_BUZZER,
	PIN_LED_DEG,
	PIN_SVDIR,
	PIN_SVGSW,
	PIN_TOTAL,	// for check total IO
	
	// custom key name
	PIN_LMUP = PIN_I3,
	PIN_LMDOWN = PIN_I4,
	PIN_LED_LMUP = PIN_Q2,
	PIN_LED_LMDOWN = PIN_Q3,
} HW_PIN_NAME_t;

extern HW_PIN_t pinList[PIN_TOTAL];

#endif /* END FILE */
