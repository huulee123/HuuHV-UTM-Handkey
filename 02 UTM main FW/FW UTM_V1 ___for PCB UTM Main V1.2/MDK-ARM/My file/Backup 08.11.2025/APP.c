/**
  ******************************************************************************
  * @Date           : 03.11.2025
  * @Author			: Trong Huu
  * @Brief          : Main program header
  ******************************************************************************
  * @Version
  + v1.1 : 06.11.2025, led bat is online bat

  * @Note
	Check volt 2 bat
	If higher limit then set online, other will charging
	If all low battery will chager for higher volt bat

  ******************************************************************************
  */

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "APP.h"
#include "LOG_DEG\LOG_DEG.h"
#include "ADS1115.h"
#include "string.h"

// Define CMD maker here
const char *cmdList[7] = 
{
	"FUP", 	// Fast Up
	"FDN",	// Fast Down
	"FSP", 	// Fast Stop
	"MUP",	// Move Up
	"MDN",	// Move Down
	"MSP",	// STOP
	"GST"	// Get status
};
const char *cmdFbHeaderStr = "OK"; 		// Example: OKFUP

extern TIM_HandleTypeDef htim1;		// PWM
extern TIM_HandleTypeDef htim2; 	// ISR check led
extern TIM_HandleTypeDef htim3; 	// buzzer
extern TIM_HandleTypeDef htim4; 	// check error rx buffer
extern UART_HandleTypeDef huart3;

// GLOBAL VARIABLE
UTM_MOVE_TYPE_t move = MOVE_NONE;
SW_SERVO_CONFIG_t swConf;					// config by SWDIP2
STATUS_t stt;						// status of machine

/* RX BUFFER UART */
char gUartBuff[UART_RX_MAX_SIZE]; 	// incomming cmd
char gRxBuff[UART_RX_MAX_SIZE]; 		// incomming cmd
volatile uint8_t gRxSize = 0;

volatile int cmdSpeed = 0;
volatile uint8_t cmd = CMD_NONE;
uint64_t rxNewCmdCnt = 0; 			// check in cycle for monitor error if no rx in 1s time

int arrData[SPEED_REF_MAX + 1] = {1000};
int preData[SPEED_REF_MAX + 1] = {1000};

void uartPutData(const char *fmt, ...)
{
	char cmdBuff[100];
	char sendBuff[128];
	va_list arglist;

	// Tạo nội dung payload
	va_start(arglist, fmt);
	vsprintf(cmdBuff, fmt, arglist);
	va_end(arglist);

	// Tính checksum
	uint8_t sum = checkSum((const uint8_t*)cmdBuff, strlen(cmdBuff));

	// {payloadCHK}\r\n
	sprintf(sendBuff, "{%s%03d}\r\n", cmdBuff, sum);

	// Gửi UART
	HAL_UART_Transmit_IT(&huart3, (uint8_t*)sendBuff, strlen(sendBuff));
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
		
		for (uint8_t i = 0; i < CMD_TOTAL; i++)
		{
			if (strncmp(cmdStr, cmdList[i], 3) == 0)
			{
				if (i != CMD_GET_STATUS)
					LOG_DEG("\nReceived payload: [%s]", payload);
				cmd = i;
				uint8_t status = 100 * stt.running + 10 * stt.lmUp + stt.lmDown;
				uartPutData("%s%s%03d", cmdFbHeaderStr, cmdStr, status);
				return;
			}
		}
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == huart3.Instance)
	{
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
			stt.lmUp = 1;
			PIN_SET(PIN_LED_LMUP);
		}
		else
		{
			stt.lmUp = 0;
			PIN_RESET(PIN_LED_LMUP);
		}

		if (pinReadFilter(PIN_LMDOWN))
		{
			stt.lmDown = 1;
			PIN_SET(PIN_LED_LMDOWN);
		}
		else
		{
			stt.lmDown = 0;
			PIN_RESET(PIN_LED_LMDOWN);
		}

		if (move != MOVE_NONE)
			stt.running = 1;
		else
			stt.running = 0;	
	}
	else if (htim->Instance == htim3.Instance) // buzer
	{
		PIN_RESET(PIN_BUZZER);
		HAL_TIM_Base_Stop(&htim3);
	}
	else if (htim->Instance == htim4.Instance) // check rx new cmd counter
	{
		HAL_GPIO_TogglePin(LED_DEG_GPIO_Port, LED_DEG_Pin); // Debug
		
		if (rxNewCmdCnt == 0)
		{
			__HAL_UART_CLEAR_OREFLAG(&huart3); // clear all error of rx buffer
		}
		
		rxNewCmdCnt = 0; // reset check
	}
}

void APP_Init(void)
{
	HAL_Delay(50);
	LOG_DEG("\n\n---- FIRMWARE UTM-V1 FOR PCB UTM-Main-V1.2 -----\n");
	
	// Start PWM
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start_IT(&htim4);
	
	__HAL_UART_CLEAR_OREFLAG(&huart3); // clear all error of rx buffer
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, (uint8_t *)gUartBuff, UART_RX_MAX_SIZE);
	
	svStop(0);

	// SW config
	if (PIN_READ(PIN_SW1) == 0)
		swConf.dir = 1;
	else
		swConf.dir = 0;
	
	if (PIN_READ(PIN_SW2) == 0)
		swConf.fMax = 500000;
	else
		swConf.fMax = 200000;
	
	LOG_DEG("\nDir = %d, Fmax = %d Hz", swConf.dir, swConf.fMax);
	////    INIT VALUE OFF DATA TIMER PWM        ///
	preData[1] = 12;
	arrData[1] = 60000;
	for (int i = 2; i <= 1000; i++)
	{
		timPreArrCalculate(i);
	}
	
	LOG_DEG("\nFinish init");
}

void APP_Execute(void)
{
	while (1)
	{
		move = getAction();
		
		if (move != MOVE_NONE)
		{
			LOG_DEG("\nStart move ");
			cmd = CMD_NONE;
			
			/* set dir of servo */
			if (move == MOVE_FAST_UP || move == MOVE_UP || move == MOVE_BIT0_UP)
				svDirSet(SV_DIR_UP);
			else
				svDirSet(SV_DIR_DOWN);

			/* Fast up and down with constant speed */
			if (move == MOVE_FAST_UP || move == MOVE_FAST_DOWN)
			{
				LOG_DEG("fast up down");
				buzzerOn();
				svGswSet(1); // servo gear switch for fast speed
				
				LOG_DEG("\n\t__ speed up");
				for (int spd = 1, step = 0; spd <= SPEED_REF_MAX; spd += step * SPEED_STEP_ACC_DEC)
				{
					svSpeedSet(spd);
					HAL_Delay(2);
					step++;
				}
				svSpeedSet(SPEED_REF_MAX);

				LOG_DEG("\n\t__ loop check exit");
				while (1)
				{
					if (cmd == CMD_STOP || cmd == CMD_FAST_STOP)
					{
						LOG_DEG("\n\t__ by cmd stop");
						break;
					}
					
					if (move == MOVE_FAST_UP && pinReadFilter(PIN_LMUP))
					{
						LOG_DEG("\n\t__ by limit");
						break;
					}
					
					if (move == MOVE_FAST_DOWN && pinReadFilter(PIN_LMDOWN))
					{
						LOG_DEG("\n\t__ by limit");
						break;
					}
				}

				move = MOVE_NONE;
				svStop(cmdSpeed);
				svGswSet(0);
				LOG_DEG("\n\t__ end");
			}
			
			/* Loading and Release with spd speed */
			else if (move == MOVE_UP || move == MOVE_DOWN)
			{
				LOG_DEG("up down");
				buzzerOn();
				
				int spd = 0;
				int spdOld = 0; // check diff for change speed
				
				while (1)
				{
					if (abs(spd - cmdSpeed) <= SPEED_STEP_ACC_DEC)
						spd = cmdSpeed;
					else if (spd < cmdSpeed)
						spd += SPEED_STEP_ACC_DEC;
					else
						spd -= SPEED_STEP_ACC_DEC;
					
					if (spd != spdOld)
					{
						svSpeedSet(spd);
						spdOld = spd;
						HAL_Delay(1);
					}

					// Check stop
					if (cmd == CMD_STOP)
						break;
					
					if (move == MOVE_UP && pinReadFilter(PIN_LMUP))
						break;
					
					if (move == MOVE_DOWN && pinReadFilter(PIN_LMDOWN))
						break;
				}

				move = MOVE_NONE; // for off led
				buzzerOn();

				// Stop servo
				svStop(spd);
			}
			
			/* CPU control: Bit 0 & Bit 1 with Analog output speed */
			else // (action == MOVE_BIT0 || action == MOVE_BIT1)
			{
				LOG_DEG("computer");
				buzzerOn();
				
				uint32_t tick = HAL_GetTick();
				uint8_t readyAO = 0; // wait 2s for start speed from software
				uint8_t started = 0;
				
				// read to buff
				unsigned int adcBuff = 0;
				unsigned int adcValue = 0;
				int spdConvert;
				ADS1115_Init(ADC_CH_AO);

				while (1)
				{
					if (started == 0)
					{
						if (readyAO == 0) // wait software output analog
						{
							if (HAL_GetTick() - tick > 2000)
								readyAO = 1;
						}
						else
						{
							for (uint8_t i = 0; i < 100; i++)
							{
								adcBuff += ADS1115_ReadData(); // change to CH1
							}
							adcValue = (float) adcBuff / 100.0;

							// smoot speed
							spdConvert = adcValue * SPEED_REF_MAX / 32000.0;
							for (int i = 1, step = 0; i <= spdConvert; i += step * SPEED_STEP_ACC_DEC)
							{
								svSpeedSet(i);
								HAL_Delay(2);
								step++;
							}
							svSpeedSet(spdConvert);
							started = 1;
						}
					}
					
					if (move == MOVE_BIT0_UP)
					{
						if (!pinReadFilter(PIN_BIT0) || pinReadFilter(PIN_LMUP)) 
							break;
					}
					else // bit 1-Release
					{
						if (!pinReadFilter(PIN_BIT0) || pinReadFilter(PIN_LMDOWN)) 
							break;
					}
				}

				move = MOVE_NONE; // for off led
				buzzerOn();

				// Stop servo
				svStop(spdConvert); // speed down to 0 from current speed
			}
		}
	} // end while
}

UTM_MOVE_TYPE_t getAction(void)
{
	if (pinReadFilter(PIN_BIT0) && !pinReadFilter(PIN_LMUP))
	{
		LOG_DEG("\n\nNew computer");
		return MOVE_BIT0_UP;
	}
	else if (pinReadFilter(PIN_BIT1) && !pinReadFilter(PIN_LMDOWN))
	{
		LOG_DEG("\n\nNew computer");
		return MOVE_BIT1_DOWN;
	}
	else if (cmd == CMD_FAST_UP && !pinReadFilter(PIN_LMUP))
	{
		LOG_DEG("\n\nNew fast up");
		return MOVE_FAST_UP;
	}
	else if (cmd == CMD_FAST_DOWN && !pinReadFilter(PIN_LMDOWN))
	{
		LOG_DEG("\n\nNew fast down");
		return MOVE_FAST_DOWN;
	}
	else if (cmd == CMD_MOVE_UP && !pinReadFilter(PIN_LMUP))
	{
		LOG_DEG("\n\nNew up");
		return MOVE_UP;
	}
	else if (cmd == CMD_MOVE_DOWN && !pinReadFilter(PIN_LMDOWN))
	{
		LOG_DEG("\n\nNew down");
		return MOVE_DOWN;
	}
	// return default
	return MOVE_NONE;
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

void svGswSet(int onOff)
{
	if (onOff)
		PIN_SET(PIN_SVGSW);
	else
		PIN_RESET(PIN_SVGSW);
}

void svDirSet(uint8_t flagBit)
{
	if (flagBit)
	{
		if (swConf.dir)
			PIN_SET(PIN_SVDIR);
		else
			PIN_RESET(PIN_SVDIR);
	}
	else
	{
		if (swConf.dir)
			PIN_RESET(PIN_SVDIR);
		else
			PIN_SET(PIN_SVDIR);
	}
}

void svStop(uint16_t adc)
{
	for (int i = adc; i > 0; i -= SPEED_STEP_ACC_DEC)
	{
		svSpeedSet(i);
		HAL_Delay(2);
	}

	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0); // duty = 0
}

void svSpeedSet(int spd)
{
	if (spd <= 0)
	{
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0); // duty = 0
		return;
	}

	__HAL_TIM_SET_PRESCALER(&htim1, preData[spd] - 1);
	__HAL_TIM_SET_AUTORELOAD(&htim1, arrData[spd] - 1);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, arrData[spd]/2.0); // duty to 50%
}

void timPreArrCalculate(int spd)
{	
	float freq = (float) spd * swConf.fMax / SPEED_REF_MAX;
	//debFreq = freq;
	for (int pre = 1; pre < 10; pre++)
	{
		int arr = 72000000.0 / (freq * pre);
		if (arr >= 2 && arr <= 65535)
		{
			preData[spd] = pre;
			arrData[spd] = arr;
			break;
		}
	}
}

uint8_t checkSum(const uint8_t *s, uint8_t len)
{
	uint8_t num = 0;
	for (uint8_t i = 0; i < len; i++)
		num += s[i];
	return num;
}

