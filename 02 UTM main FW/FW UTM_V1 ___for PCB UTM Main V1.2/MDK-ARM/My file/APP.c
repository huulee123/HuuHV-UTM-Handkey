/**
  ******************************************************************************
  * @Date           : 08.11.2025
  * @Author			: Trong Huu
  * @Brief          : Main program header
  ******************************************************************************
  * @Version
  + v1 : 08.11.2025
		- Move by computer will wait 1.5s for analog output is updated, then read
				and conver to speed for speed up servo
		- Move by fast up down will run max speed (=1000 ref speed) during GSW servo
				is turn on for invidual move frame speed
		- Move by up down will run and update speed real time so user can change
				speed during servo running

  * @Note
		- SW1 dir set invert
		- SW2 set freq pwm max (OFF 200kHz, ON 500kHz)
  ******************************************************************************
  */

#include <stdio.h>
#include <stdlib.h> // atoi
#include <string.h> // mem
#include "stm32f1xx_hal.h"
#include "APP.h"
#include "LOG_DEG\LOG_DEG.h"
#include "ADS1115.h"

// Define CMD maker here
const char *cmdList[CMD_TOTAL] = 
{
	"FUP", 	// Fast Up
	"FDN",	// Fast Down
	"FSP", 	// Fast Stop
	"MUP",	// Move Up
	"MDN",	// Move Down
	"MSP",	// STOP
	"GST"	// Get status
};

extern TIM_HandleTypeDef htim1;		// PWM
extern TIM_HandleTypeDef htim2; 	// ISR check led
extern TIM_HandleTypeDef htim3; 	// buzzer
extern TIM_HandleTypeDef htim4; 	// check counter of rx buffer for error reset
extern UART_HandleTypeDef huart3;

// GLOBAL VARIABLE
UTM_MOVE_TYPE_t gMove = MOVE_NONE;
SW_SERVO_CONFIG_t gSwConf;			// config by SWDIP2
STATUS_t gStt;						// status of machine

/* RX BUFFER UART */
char gUartBuff[UART_RX_MAX_SIZE]; 	// incomming cmd
char gRxBuff[UART_RX_MAX_SIZE]; 		// incomming cmd
volatile uint8_t gRxSize = 0;

const uint8_t speedTableTotal = 4;
const int16_t speedTable[speedTableTotal] = {10, 30, 100, 500}; // max 1000
volatile int gCmdSpeed = 0;
volatile CMD_t gCmd = CMD_NONE;
uint64_t gCounterRx = 0; 			// check in cycle for monitor error if no rx in 1s time

uint16_t gArrReg[SPEED_REF_MAX + 1] = {0};
uint16_t gPreReg[SPEED_REF_MAX + 1] = {0};

void cmdFeedbackPut(CMD_t cmd)
{
	uint16_t len;
	char payloadBuff[20], txBuff[64];
	uint8_t status = 100 * gStt.running + 10 * gStt.lmUp + gStt.lmDown;
	len = sprintf(payloadBuff, "OK%s%03d", cmdList[cmd], status); // payload
	len = sprintf(txBuff, "{%s%03d}\r\n", payloadBuff, checkSum((const uint8_t*)payloadBuff, len)); // frame
	HAL_UART_Transmit_IT(&huart3, (uint8_t*)txBuff, len);
}

void cmdProcess(void)
{
	uint8_t payloadLen = gRxSize - 7;  // {payload,CHK(3),}\r\n
	char payload[UART_RX_MAX_SIZE - 7];
	strncpy(payload, &gRxBuff[1], payloadLen);
	payload[payloadLen] = '\0';
	char chkStr[4] = {0};
	strncpy(chkStr, (char*)&gRxBuff[1 + payloadLen], 3);
	uint16_t recvChk = atoi(chkStr);

	if (recvChk == checkSum((uint8_t*)payload, payloadLen))
	{
		// FUP12345
		char spdStr[6], cmdStr[4];
		strncpy(cmdStr, &payload[0], 3);
		strncpy(spdStr, &payload[3], 5);
		
		for (CMD_t i = CMD_FIRST; i < CMD_TOTAL; i++)
		{
			if (strncmp(cmdStr, cmdList[i], 3) == 0)
			{
				if (i != CMD_GET_STATUS)
					LOG_DEG("\nHankey >> [%s]", payload);
				gCmd = i;
				int num = atoi(spdStr);
				if (num >= 0 && num <= SPEED_REF_MAX)
					gCmdSpeed = num;
				cmdFeedbackPut(i);
				return;
			}
		}
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == huart3.Instance)
	{
		gCounterRx++;
		memcpy(gRxBuff, gUartBuff, Size); gRxBuff[Size] = '\0';
		gRxSize = Size;
		if (gRxBuff[0] == '{' || gRxBuff[gRxSize - 3] == '}' || gRxBuff[gRxSize - 2] == '\r' || gRxBuff[gRxSize - 1] == '\n')
		{
			cmdProcess();
		}
		HAL_UARTEx_ReceiveToIdle_DMA(&huart3, (uint8_t*)gUartBuff, UART_RX_MAX_SIZE);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == htim2.Instance) // 50hz : check status
	{
		// update status
		if (pinReadFilter(PIN_LMUP))
		{
			gStt.lmUp = 1;
			PIN_SET(PIN_LED_LMUP);
		}
		else
		{
			gStt.lmUp = 0;
			PIN_RESET(PIN_LED_LMUP);
		}

		if (pinReadFilter(PIN_LMDOWN))
		{
			gStt.lmDown = 1;
			PIN_SET(PIN_LED_LMDOWN);
		}
		else
		{
			gStt.lmDown = 0;
			PIN_RESET(PIN_LED_LMDOWN);
		}

		if (gMove != MOVE_NONE)
			gStt.running = 1;
		else
			gStt.running = 0;	
	}
	else if (htim->Instance == htim3.Instance) // buzer
	{
		PIN_RESET(PIN_BUZZER);
		HAL_TIM_Base_Stop(&htim3);
	}
	else if (htim->Instance == htim4.Instance) // check rx new cmd counter
	{
		HAL_GPIO_TogglePin(LED_DEG_GPIO_Port, LED_DEG_Pin); // Debug
		
		if (gCounterRx == 0)
		{
			LOG_DEG("\nError communication, start re-init communication");
			__HAL_UART_CLEAR_OREFLAG(&huart3); // clear all error of rx buffer
		}
		
		gCounterRx = 0; // reset check
	}
}

void APP_Init(void)
{
	HAL_Delay(50);
	LOG_DEG("\n\n---- FIRMWARE UTM-V1 FOR PCB UTM-Main-V1.2 -----\n");
	
	// Start PWM
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0); // duty = 0
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start_IT(&htim4);
	
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, (uint8_t *)gUartBuff, UART_RX_MAX_SIZE);

	// SW config
	if (PIN_READ(PIN_SW1) == 0)
		gSwConf.dir = 1;
	else
		gSwConf.dir = 0;
	
	if (PIN_READ(PIN_SW2) == 0)
		gSwConf.fMax = 500000;
	else
		gSwConf.fMax = 200000;
	
	LOG_DEG("\nDir = %d, Fmax = %d Hz", gSwConf.dir, gSwConf.fMax);
	////    INIT VALUE OFF DATA TIMER PWM        ///
	for (int16_t i = 0; i <= 1000; i++)
	{
		svSpeedParamCalculate(i);
		LOG_DEG("\nPRE[%4d] = %5d, ARR[%4d] = %5d", i, gPreReg[i], i, gArrReg[i]);
		HAL_Delay(1);
	}
	
	LOG_DEG("\nFinish init");
}

void APP_Execute(void)
{
	while (1)
	{
		gMove = moveCheck();
		if (gMove != MOVE_NONE)
		{
			gCmd = CMD_NONE;
			
			/* set dir of servo */
			if (gMove == MOVE_FAST_UP || gMove == MOVE_UP || gMove == MOVE_BIT0_UP)
				svDirSet(SV_DIR_UP);
			else
				svDirSet(SV_DIR_DOWN);

			/* Fast up and down with constant speed */
			if (gMove == MOVE_FAST_UP || gMove == MOVE_FAST_DOWN)
			{
				LOG_DEG("\n\t\t -- Begin Fast Up / Down --");
				buzzerOn();
				PIN_SET(PIN_SVGSW);
				svSpeedUp(SPEED_REF_MAX);
				while (1)
				{
					if (gCmd == CMD_STOP || gCmd == CMD_FAST_STOP)	break;
					if (gMove == MOVE_FAST_UP && pinReadFilter(PIN_LMUP))	break;
					if (gMove == MOVE_FAST_DOWN && pinReadFilter(PIN_LMDOWN))	break;
				}

				gMove = MOVE_NONE;
				svSpeedStop(SPEED_REF_MAX);
				PIN_RESET(PIN_SVGSW);
				LOG_DEG("\n\t\t end\n");
			}
			
			/* Loading and Release with spd speed */
			else if (gMove == MOVE_UP || gMove == MOVE_DOWN)
			{
				LOG_DEG("\n\t\t -- Begin Move Up / Down --");
				buzzerOn();
				
				int16_t spd = gCmdSpeed;
				int16_t spdOld = spd;
				int16_t spdStep = 20;
				svSpeedUp(spd);
				while (1)
				{
					if (abs(spd - gCmdSpeed) <= spdStep)
						spd = gCmdSpeed;
					else if (spd < gCmdSpeed)
						spd += spdStep;
					else
						spd -= spdStep;
					
					if (spd != spdOld)
					{
						svSpeedSet(spd);
						spdOld = spd;
						HAL_Delay(1);
						LOG_DEG("\n\t\t speed: %5d, target: %5d", spd, gCmdSpeed);
					}

					// Check stop
					if (gCmd == CMD_STOP) break;
					if (gMove == MOVE_UP && pinReadFilter(PIN_LMUP)) break;
					if (gMove == MOVE_DOWN && pinReadFilter(PIN_LMDOWN)) break;
				}

				gMove = MOVE_NONE; // for off led
				buzzerOn();
				svSpeedStop(spd);
				LOG_DEG("\n\t\t end\n");
			}
			
			/* CPU control: Bit 0 & Bit 1 with Analog output speed */
			else // (action == MOVE_BIT0 || action == MOVE_BIT1)
			{
				LOG_DEG("\n\t\t -- Begin Move Computer --");
				buzzerOn();
				
				uint32_t tick = HAL_GetTick();
				uint8_t aoWaitting = 1; // wait 2s for start speed from software
				uint8_t aoProcessing = 1;

				int32_t adcBuff = 0;
				uint8_t adcBuffIdx = 0;
				int32_t adcValue = 0;
				int32_t adcToSpeed = 0;
				ADS1115_Init(ADC_CH_AO);
				while (1)
				{
					if (aoProcessing)
					{
						if (aoWaitting) // wait software output analog
						{
							if (HAL_GetTick() - tick > 1500)
							{
								LOG_DEG("\n\t\t Analog output ready");
								aoWaitting = 0;
							}
						}
						else
						{
							adcBuff += ADS1115_ReadData(); // change to CH1
							adcBuffIdx++;
							if (adcBuffIdx >= 100)
							{
								adcValue = adcBuff / adcBuffIdx;
								LOG_DEG("\n\t\t adc read %5d", adcValue);
								if (adcValue < 0 || adcValue >= INT16_MAX)
								{
									buzzerOn();
									HAL_Delay(100);
									LOG_DEG("\n\t\t AO out of range of ADC");
									buzzerOn();
									HAL_Delay(100);
									break;
								}
								// smoot speed
								adcToSpeed = adcValue * SPEED_REF_MAX / (INT16_MAX - 1);
								svSpeedUp(adcToSpeed);
								aoProcessing = 0;
							}
						}
					}
					
					if (gMove == MOVE_BIT0_UP)
					{
						if (!pinReadFilter(PIN_BIT0) || pinReadFilter(PIN_LMUP)) break;
					}
					else // bit 1-Release
					{
						if (!pinReadFilter(PIN_BIT1) || pinReadFilter(PIN_LMDOWN)) break;
					}
				}

				gMove = MOVE_NONE; // for off led
				buzzerOn();
				svSpeedStop(adcToSpeed); // speed down to 0 from current speed
				LOG_DEG("\n\t\t end\n");
			}
		}
	} // end while
}

UTM_MOVE_TYPE_t moveCheck(void)
{
	if (pinReadFilter(PIN_BIT0) && !pinReadFilter(PIN_LMUP))
	{
		LOG_DEG("\n\n[Computer]");
		return MOVE_BIT0_UP;
	}
	else if (pinReadFilter(PIN_BIT1) && !pinReadFilter(PIN_LMDOWN))
	{
		LOG_DEG("\n\n[Computer]");
		return MOVE_BIT1_DOWN;
	}
	else if (gCmd == CMD_FAST_UP && !pinReadFilter(PIN_LMUP))
	{
		LOG_DEG("\n\n[Fast up]");
		return MOVE_FAST_UP;
	}
	else if (gCmd == CMD_FAST_DOWN && !pinReadFilter(PIN_LMDOWN))
	{
		LOG_DEG("\n\n[Fast down]");
		return MOVE_FAST_DOWN;
	}
	else if (gCmd == CMD_MOVE_UP && !pinReadFilter(PIN_LMUP))
	{
		LOG_DEG("\n\n[Move up]");
		return MOVE_UP;
	}
	else if (gCmd == CMD_MOVE_DOWN && !pinReadFilter(PIN_LMDOWN))
	{
		LOG_DEG("\n\n[Move down]");
		return MOVE_DOWN;
	}
	// return default
	return MOVE_NONE;
}

void svSpeedUp(int16_t spdTarget)
{
	LOG_DEG("\n\t\t Accelerate");
	for (uint8_t i = 0; i < speedTableTotal; i++)
	{
		if (speedTable[i] > spdTarget)
			break;
		svSpeedSet(speedTable[i]);
		HAL_Delay(2);
		LOG_DEG("\n\t\t speed %5d / %5d", speedTable[i], spdTarget);
	}
	svSpeedSet(spdTarget);
	LOG_DEG("\n\t\t speed %5d / %5d", spdTarget, spdTarget);
}

void svSpeedStop(int16_t spdCurrent)
{
	LOG_DEG("\n\t\t Decelerate");
	for (int8_t i = speedTableTotal - 1; i >= 0; i--)
	{
		if (speedTable[i] > spdCurrent)
			continue;
		svSpeedSet(speedTable[i]);
		HAL_Delay(2);
		LOG_DEG("\n\t\t speed %5d / 0", speedTable[i]);
	}
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0); // duty = 0
	LOG_DEG("\n\t\t stopped");
}

uint8_t pinReadFilter(HW_PIN_NAME_t pin)
{
	for (uint8_t i = 0; i <= 15; i++)
	{
		if (PIN_READ(pin)) // != 0 is not press
			return 0;
		__NOP();
	}
	
	return 1;
}

void buzzerOn(void)
{
	PIN_SET(PIN_BUZZER);
	HAL_TIM_Base_Start_IT(&htim3); // start timer to stop buzzer
	// buzzer ontime = timer cycle
}

void svDirSet(uint8_t flagBit)
{
	if (flagBit)
	{
		if (gSwConf.dir)
			PIN_SET(PIN_SVDIR);
		else
			PIN_RESET(PIN_SVDIR);
	}
	else
	{
		if (gSwConf.dir)
			PIN_RESET(PIN_SVDIR);
		else
			PIN_SET(PIN_SVDIR);
	}
}

void svSpeedSet(int16_t spd)
{
	if (spd <= 0 || spd > SPEED_REF_MAX)
	{
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0); // duty = 0
		return;
	}

	__HAL_TIM_SET_PRESCALER(&htim1, gPreReg[spd] - 1);
	__HAL_TIM_SET_AUTORELOAD(&htim1, gArrReg[spd] - 1);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, gArrReg[spd]/2.0); // duty to 50%
}

void svSpeedParamCalculate(int16_t spd)
{
	float freq = (float)spd * gSwConf.fMax / SPEED_REF_MAX;
    if (freq < 1.0f) freq = 1.0f;  // tránh chia 0

    float targetPeriod = 72000000.0f / freq;
    uint16_t pre = 1;

    while (targetPeriod / pre > 65535 && pre < 65535)
        pre++;

    uint16_t arr = (uint16_t)(targetPeriod / pre);

    gPreReg[spd] = pre;
    gArrReg[spd] = arr;
}

uint8_t checkSum(const uint8_t *s, uint8_t len)
{
	uint8_t num = 0;
	for (uint8_t i = 0; i < len; i++)
		num += s[i];
	return num;
}

