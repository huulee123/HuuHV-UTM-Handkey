/**
  ******************************************************************************
  * @Date           : 03.11.2025
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

/// USER DEFINE
#define SV_DIR_UP	0 // for motor dir set
#define SV_DIR_DOWN	1

#define SPEED_REF_MAX		1000	// 100% speed
#define UART_RX_MAX_SIZE	128

typedef enum
{
    MOVE_FAST_UP = 0,
    MOVE_FAST_DOWN,
	MOVE_UP,
	MOVE_DOWN,
    MOVE_BIT0_UP,
    MOVE_BIT1_DOWN,
    MOVE_NONE
} UTM_MOVE_TYPE_t;

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
	CMD_NONE,
	CMD_FIRST = CMD_FAST_UP
} CMD_t;

typedef struct
{
	uint32_t fMax;	// freq PWM max
	uint8_t dir;		// dir of motor
} SW_SERVO_CONFIG_t;

typedef struct
{
	uint8_t lmUp;
	uint8_t lmDown;
	uint8_t running;
} STATUS_t;

void cmdFeedbackPut(CMD_t cmd);
uint8_t checkSum(const uint8_t* s, uint8_t len);

void buzzerOn(void);
uint8_t pinReadFilter(HW_PIN_NAME_t pin);
UTM_MOVE_TYPE_t moveCheck(void);

void svDirSet(uint8_t flagBit);
void svSpeedUp(int16_t spdTarget);
void svSpeedStop(int16_t spdCurrent);
void svSpeedSet(int16_t spd);
void svSpeedParamCalculate(int16_t spd);

// FUNCTION
void APP_Init(void);
void APP_Execute(void);

#endif /* END FILE */
