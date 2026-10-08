/**
  ******************************************************************************
  * @Date           : 28.10.2025
  * @Author			: Trong Huu
  * @Brief          : Main program header
  ******************************************************************************
  * @UpdateLog


  * @Note


  ******************************************************************************
  */

#ifndef APP_H_
#define APP_H_

#include "main.h"
#include "HWIO.h"

#define SPEED_FLASH_ADDRESS		FLASH_USER_START_ADDR
#define SPEED_SET_MAX			1000 // 100.0%
#define UART_RX_MAX_SIZE		128
#define UART_TX_MAX_SIZE		64
#define PAYLOAD_MAX_SIZE		30

typedef enum
{
	CMD_FAST_UP = 0,
	CMD_FAST_DOWN,
	CMD_FAST_STOP,
	CMD_MOVE_UP,
	CMD_MOVE_DOWN,
	CMD_STOP,
	CMD_GET_STATUS,
	CMD_TOTAL,
	CMD_NONE
} CMD_t;

//void doBuz(uint8_t state);
//void buzzerOn(void);
void cursorDisplay(int8_t x);
void statusDisplay(char *s);
void speedDisplay(int speed);

void cmdPut(CMD_t cmd);
void cmdProcess(void);
uint8_t checkSum(const uint8_t* s, uint8_t len);

// FUNCTION
void APP_Init(void);
void APP_Execute(void);

#endif /* UTM_H_ */
