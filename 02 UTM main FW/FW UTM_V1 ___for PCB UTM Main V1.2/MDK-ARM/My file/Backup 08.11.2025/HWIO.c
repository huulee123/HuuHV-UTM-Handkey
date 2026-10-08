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
    {SW1_GPIO_Port, SW1_Pin},
	{SW2_GPIO_Port, SW2_Pin},
	{I0_GPIO_Port, I0_Pin},
	{I1_GPIO_Port, I1_Pin},
	{I2_GPIO_Port, I2_Pin},
	{I3_GPIO_Port, I3_Pin},
	{I4_GPIO_Port, I4_Pin},
	{Q0_GPIO_Port, Q0_Pin},
	{Q1_GPIO_Port, Q1_Pin},
	{Q2_GPIO_Port, Q2_Pin},
	{Q3_GPIO_Port, Q3_Pin},
	{BIT0_GPIO_Port, BIT0_Pin},
	{BIT1_GPIO_Port, BIT1_Pin},
	{BUZZER_GPIO_Port, BUZZER_Pin},
	{LED_DEG_GPIO_Port, LED_DEG_Pin},
	{SV_DIR_GPIO_Port, SV_DIR_Pin},
	{SV_GSW_GPIO_Port, SV_GSW_Pin},
};

