
#include "FLASH.h"

void FLASH_Write(uint32_t address, int num)
{
    HAL_FLASH_Unlock();

    // Xóa 1 page (1KB)
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t PageError = 0;

    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = address;
    eraseInit.NbPages = 1;
    HAL_FLASHEx_Erase(&eraseInit, &PageError);

    // Ghi dữ liệu (4 byte)
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, (uint32_t)num);

    HAL_FLASH_Lock();
}

int FLASH_Read(uint32_t address)
{
    uint32_t val = *(__IO uint32_t*)address;

    // Nếu vùng Flash chưa ghi (toàn 0xFFFFFFFF)
    if (val == 0xFFFFFFFF)
        return 0;   // giá trị mặc định
    else
        return (int)val;
}

