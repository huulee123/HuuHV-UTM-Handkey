/**
  ******************************************************************************
  * @Date           : 28.10.2025
  * @Author			: Trong Huu
  * @Brief          : Main program body
  ******************************************************************************
  * @UpdateLog
  + V1 : 10.11.2025
		- Check no data feedback every 2s to re-init uart port
		- Auto get status every 250ms during free time
		- Write new speed value to Flash every 20s after change speed
  * @Note

	+ Handkey is Master
	+ Master send:
	--> Send 		{FUPxxxxxSUM}\r\n     	:	xxxxx is speed percent, SUM is check sum
	<-- Feedback	{OKFUPrLLSUM}\r\n		:	r is Running/stop, LL is Limit UpDown status
  ******************************************************************************
  */

#include <stdio.h> // sprintf
#include <string.h> // mem api
#include <stdlib.h> // atoi
#include "APP.h"
#include "KEYPAD.h"
#include "FLASH.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "stm32f1xx_hal.h"
#include "LOG_DEG\LOG_DEG.h"

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

extern TIM_HandleTypeDef htim2; // key and status led
extern UART_HandleTypeDef huart2;

// GLOBAL VARIABLE
int gSpeedSet = 200, gSpeedSetOld = 0;
uint8_t gSttRun = 0, gSttLimit = 0;
uint32_t gTick = 0, gTickGetStatus = 0, gTickCheckRx = 0;
uint32_t gCounterRx = 0;

/* RX BUFFER UART */
char gRxBuff[UART_RX_MAX_SIZE]; 		// from DMA
char gRxData[UART_RX_MAX_SIZE]; 		// user process in
volatile uint8_t gRxSize = 0;
char gTxBuff[UART_TX_MAX_SIZE];		// payload + checksum + frame
char payloadTxBuff[PAYLOAD_MAX_SIZE]; 	// cmd + data
char payloadRxBuff[PAYLOAD_MAX_SIZE];	// cmd + data

void cmdProcess(void)
{
	PIN_RESET(PIN_LED_COM);
	uint8_t payloadLen = gRxSize - 7;  // {payload,CHK(3),}\r\n
	strncpy(payloadRxBuff, &gRxData[1], payloadLen);
	char chkStr[4] = {0};
	strncpy(chkStr, (char*)&gRxData[1 + payloadLen], 3);
	uint16_t recvChk = atoi(chkStr);

	if (recvChk == checkSum((uint8_t*)payloadRxBuff, payloadLen))
	{
		// FUP12345
		char sttNum[4];
		strncpy(sttNum, &payloadRxBuff[5], 3);
		uint8_t num = atoi((const char *)sttNum);
		gSttRun = num / 100;
		gSttLimit = num % 100;
	}
	PIN_SET(PIN_LED_COM);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == huart2.Instance)
	{
		gCounterRx++;
		memcpy(gRxData, gRxBuff, Size); gRxData[Size] = '\0';
		gRxSize = Size;
		if (gRxData[0] == '{' && gRxData[gRxSize - 3] == '}' && gRxData[gRxSize - 2] == '\r' && gRxData[gRxSize - 1] == '\n')
			cmdProcess();
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t*)gRxBuff, UART_RX_MAX_SIZE);
	}
}

//------------------------------------------------------------------------- TIMER: KEY
volatile uint8_t gKeyData = KEY_NONE;	// data
volatile int8_t gKeyEvent = 0;	// for check new
int8_t gKeyHold = 0;	// key is pressing down
uint16_t gKeyHoldCnt = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == htim2.Instance) // 20Hz
	{
		uint8_t key = KEY_Get();
		if (key != KEY_NONE && gKeyHold == 0)
		{
			gKeyHold = 1;
			gKeyData = key;
		}
		else if (key == KEY_NONE && gKeyHold == 1)
		{
			gKeyHold = 0; // reset state
			gKeyHoldCnt = 0;
			if (gKeyData == KEY_FUP || gKeyData == KEY_FDOWN)
			{
				gKeyData = KEY_FSTOP;
				gKeyEvent = 1;
			}
			
		}
		
		if (gKeyHold == 1)
		{
			gKeyHoldCnt++;
			if (gKeyHoldCnt == 2)
				gKeyEvent = 1;		// alarm new key
			else if (gKeyHoldCnt % 6 == 0)
			{
				if (gKeyData == KEY_ADD || gKeyData == KEY_SUB)
					gKeyEvent = 1;	// repeat key
			}
		}
		
		//// LED STATUS
		if (gSttRun)
			PIN_RESET(PIN_LED_RUN);
		else
			PIN_SET(PIN_LED_RUN);
		
		if (gSttLimit)
			PIN_RESET(PIN_LED_LIMIT);
		else
			PIN_SET(PIN_LED_LIMIT);
	}
}

void APP_Init(void)
{
	HAL_Delay(50);
	LOG_DEG("\n\n---- FIRMWARE HANKEY-V1 FOR PCB-HANKEY-V0.2 -----\n");
	PIN_RESET(PIN_LED_PWR); // Led power

	ssd1306_Init();
	ssd1306_Fill(COLOR_BLACK);
	statusDisplay("SPEED");
	ssd1306_SetCursor(115, 30);
	ssd1306_WriteString("%", Font_11x18, COLOR_WHITE);
	
	LOG_DEG("\nFlash get ...");
	gSpeedSet = FLASH_Read(SPEED_FLASH_ADDRESS);
	gSpeedSetOld = gSpeedSet;
	if (gSpeedSet > 1000)
		gSpeedSet = 1000;
	else if (gSpeedSet < 0)
		gSpeedSet = 10;
	LOG_DEG(" ok");
	
	HAL_Delay(2000);
	HAL_TIM_Base_Start_IT(&htim2);		// keypad
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t*)gRxBuff, UART_RX_MAX_SIZE);
	LOG_DEG("\nFinish init");
}

void APP_Execute(void)
{
	uint8_t key = KEY_NONE;
	int8_t posCur = 0; // 0 to 4
	uint32_t tickSpeedChange = 0;
	cursorDisplay(posCur);
	speedDisplay(gSpeedSet);
	const int stepTable[3] = {1, 10, 100};
	
	while (1)
	{
		gTick = HAL_GetTick();
		if (gKeyEvent)
		{
			LOG_DEG("\nNew key press");
			key = gKeyData;
			gKeyEvent = 0;
			switch(key)
			{
				case KEY_ADD:
					gSpeedSet += stepTable[posCur];
					if (gSpeedSet >= SPEED_SET_MAX)
						gSpeedSet = SPEED_SET_MAX;
					speedDisplay(gSpeedSet);
					tickSpeedChange = gTick; // recent change time
					LOG_DEG(" speed %d", gSpeedSet);
					break;
					
				case KEY_SUB:
					gSpeedSet -= stepTable[posCur];
					if (gSpeedSet < 0)
						gSpeedSet = 0;
					speedDisplay(gSpeedSet);
					tickSpeedChange = gTick; // recent change time
					LOG_DEG(" speed %d", gSpeedSet);
					break;
					
				case KEY_SHIFT_SET:
					posCur++;
					if (posCur > 2)
						posCur = 0;
					cursorDisplay(posCur);
					LOG_DEG(" shift cursor");
					break;
					
				case KEY_FUP:
					LOG_DEG(" fast up");
					cmdPut(CMD_FAST_UP);
					break;
				
				case KEY_FDOWN:
					LOG_DEG(" fast down");
					cmdPut(CMD_FAST_DOWN);
					break;
				
				case KEY_FSTOP:
					LOG_DEG(" fast stop");
					cmdPut(CMD_FAST_STOP);
					break;
				
				case KEY_UP:
					LOG_DEG(" move up");
					cmdPut(CMD_MOVE_UP);
					break;
				
				case KEY_DOWN:
					LOG_DEG(" move down");
					cmdPut(CMD_MOVE_DOWN);
					break;
				
				case KEY_STOP:
					LOG_DEG(" stop");
					cmdPut(CMD_STOP);
					break;
			}
		} // end if key

		if (gTick - gTickGetStatus > 250) // check status if free time every 500ms
		{
			cmdPut(CMD_GET_STATUS);
			HAL_GPIO_TogglePin(LED_DEG_GPIO_Port, LED_DEG_Pin);
		}
		if (gTick - gTickCheckRx > 2000) // check error communication if no package incomming
		{
			if (gCounterRx < 1)
			{
				LOG_DEG("\nError communication, start re-init communication");
				// Reset UART
				__HAL_UART_CLEAR_OREFLAG(&huart2); // clear all error of rx buffer
				HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t*)gRxBuff, UART_RX_MAX_SIZE);
			}
			gCounterRx = 0;
			gTickCheckRx = gTick;
		}
		if (gTick - tickSpeedChange > 20000) // check every 10s
		{
			if (gSpeedSetOld != gSpeedSet)
			{
				LOG_DEG("\nWrite new speed to Flash");
				// Write flash
				FLASH_Write(SPEED_FLASH_ADDRESS, gSpeedSet);
				HAL_Delay(5);
				gSpeedSetOld = gSpeedSet;
			}
			tickSpeedChange = gTick;
		}
	}
}

/**********************************************************************************************************
  * @brief  user funtion
  ********************************************************************************************************/
uint8_t checkSum(const uint8_t* s, uint8_t len)
{
	uint8_t num = 0;
	for (uint8_t i = 0; i < len; i++)
		num += s[i];
	return num;
}

void cmdPut(CMD_t cmd)
{
	uint16_t len;
	len = sprintf(payloadTxBuff, "%s%05d", cmdList[cmd], gSpeedSet);
	len = sprintf(gTxBuff, "{%s%03d}\r\n", payloadTxBuff, checkSum((const uint8_t*)payloadTxBuff, len));
	HAL_UART_Transmit_IT(&huart2, (uint8_t*)gTxBuff, len);
	gTickGetStatus = gTick; // reset status get
}

//void doBuz(uint8_t state)
//{
//	if (state)
//		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
//	else
//		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
//}

//void buzzerOn(void)
//{
//	doBuz(1);
//	HAL_TIM_Base_Start_IT(&htim3); // start timer to stop buzzer
//}

void cursorDisplay(int8_t x)
{
	if (x >= 1)
		x++; // ignore .
	
	ssd1306_FillRectangle(22, 59, 18 + 16 * 4 + 16, 63, COLOR_BLACK);
	ssd1306_FillRectangle(22 + 16 * (4 - x), 59, 18 + 16 * (4 - x) + 16, 63, COLOR_WHITE);
	
	ssd1306_UpdateScreen();
}

void statusDisplay(char *s)
{
	ssd1306_SetCursor(2, 0);
	ssd1306_WriteString("            ", Font_11x18, COLOR_WHITE);
	ssd1306_SetCursor(2, 0);
	ssd1306_WriteString(s, Font_11x18, COLOR_WHITE);
	ssd1306_UpdateScreen();
}

void speedDisplay(int speed)
{
	char buff[10];
	sprintf(buff, "%5.1f", speed / 10.0);
	ssd1306_SetCursor(20, 27);
	ssd1306_WriteString(buff, Font_16x26, COLOR_WHITE);
	ssd1306_UpdateScreen();
}
