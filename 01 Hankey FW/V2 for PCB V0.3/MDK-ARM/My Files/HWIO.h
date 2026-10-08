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
	PIN_LED_COM = 0,
	PIN_LED_PWR,
	PIN_LED_RUN,
	PIN_LED_LIMIT,
	PIN_LED_DEG,
	PIN_KEY_CLK,
	PIN_KEY_LOAD,
	PIN_KEY_DATA,
	PIN_BUZZER,
	PIN_TOTAL,	// for check total IO
	
	// custom key name

} HW_PIN_NAME_t;

extern HW_PIN_t pinList[PIN_TOTAL];

#endif /* END FILE */
