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

#include "HWIO.h"
#include "stm32f1xx_hal.h"

HW_PIN_t pinList[PIN_TOTAL] = 
{
    {LED3_GPIO_Port, LED3_Pin},	// COM
	{LED4_GPIO_Port, LED4_Pin},	// Power bu MCU
	{LED2_GPIO_Port, LED2_Pin},	// Run
	{LED1_GPIO_Port, LED1_Pin},	// Limit
	{LED_DEG_GPIO_Port, LED_DEG_Pin},
	{KEY_CLK_GPIO_Port, KEY_CLK_Pin},
	{KEY_LOAD_GPIO_Port, KEY_LOAD_Pin},
	{KEY_DATA_GPIO_Port, KEY_DATA_Pin},
	{BUZZER_GPIO_Port, BUZZER_Pin},
};

