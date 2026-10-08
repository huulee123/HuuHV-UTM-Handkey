
#ifndef FLASH_H_
#define FLASH_H_

#include "stm32f1xx_hal.h"

#define FLASH_USER_START_ADDR   0x0800FC00U  // Page cuối cùng

void FLASH_Write(uint32_t address, int num);
int FLASH_Read(uint32_t address);

#endif
