#ifndef __SSD1306_H__
#define __SSD1306_H__

#include <stddef.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

/* vvv I2C config vvv */
#define SSD1306_I2C_PORT        hi2c1
#define SSD1306_I2C_ADDR        (0x3C << 1)
// SSD1306 OLED in pixels
#define SSD1306_HEIGHT          64
#define SSD1306_WIDTH           128

// Include only needed fonts
//#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26
//#define SSD1306_INCLUDE_FONT_16x24
//#define SSD1306_INCLUDE_FONT_16x15

#define SSD1306_X_OFFSET_LOWER 0
#define SSD1306_X_OFFSET_UPPER 0

#define SSD1306_BUFFER_SIZE (int)(SSD1306_WIDTH * SSD1306_HEIGHT / 8.0)

// Enumeration for screen colors
typedef enum 
{
    COLOR_BLACK = 0x00, // Black color, no pixel
    COLOR_WHITE = 0x01  // Pixel is set. Color depends on OLED
} SSD1306_COLOR;

// Struct to store transformations
typedef struct 
{
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Initialized;
    uint8_t DisplayOn;
} SSD1306_t;

/** Font */
typedef struct 
{
	const uint8_t width;                /**< Font width in pixels */
	const uint8_t height;               /**< Font height in pixels */
	const uint16_t *const data;         /**< Pointer to font data array */
    const uint8_t *const char_width;    /**< Proportional character width in pixels (NULL for monospaced) */
} SSD1306_Font_t;

// Procedure definitions
void ssd1306_Init(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
char ssd1306_WriteChar(char ch, SSD1306_Font_t Font, SSD1306_COLOR color);
char ssd1306_WriteString(char* str, SSD1306_Font_t Font, SSD1306_COLOR color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color);

/**
 * @brief Sets the contrast of the display.
 * @param[in] value contrast to set.
 * @note Contrast increases as the value increases.
 * @note RESET = 7Fh.
 */
void ssd1306_SetContrast(const uint8_t value);

/**
 * @brief Set Display ON/OFF.
 * @param[in] on 0 for OFF, any for ON.
 */
void ssd1306_SetDisplayOn(const uint8_t on);

// Low-level procedures
void ssd1306_WriteCommand(uint8_t byte);
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size);

#endif // __SSD1306_H__
