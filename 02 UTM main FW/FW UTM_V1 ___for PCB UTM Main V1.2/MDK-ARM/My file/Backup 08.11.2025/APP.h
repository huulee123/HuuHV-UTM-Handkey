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
#define ADC_READ_MAX		32767	// 16b ADC
#define SPEED_STEP_ACC_DEC	10		// step for acc & dec speed

#define UART_RX_MAX_SIZE	128

typedef enum 
{
	UART_FB_OK = 0,
	UART_FB_TIMEOUT,
	UART_FB_INVALID_FORMAT,
	UART_FB_CHECKSUM_FAIL
} UART_FB_t;

// Move action type
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

// CMD from hand key
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

// HW config
typedef struct
{
	uint32_t fMax;	// freq PWM max
	uint8_t dir;		// dir of motor
} SW_SERVO_CONFIG_t;

// Status
typedef struct
{
	uint8_t lmUp;
	uint8_t lmDown;
	uint8_t running;
} STATUS_t;

void uartPutData(const char *fmt, ...);
uint8_t checkSum(const uint8_t* s, uint8_t len);
UART_FB_t uartGetData(char *toData, uint32_t timeout);

void buzzerOn(void);
uint8_t pinReadFilter(HW_PIN_NAME_t pin);
UTM_MOVE_TYPE_t getAction(void);

void svGswSet(int onOff);
void svDirSet(uint8_t flagBit);
void svStop(uint16_t adc);
void svSpeedSet(int spd);
void timPreArrCalculate(int spd);

// FUNCTION
void APP_Init(void);
void APP_Execute(void);

#endif /* END FILE */
