#include "ssd1306.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>  // For memcpy

// Send a byte to the command register
void ssd1306_WriteCommand(uint8_t byte) 
{
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}

// Send data
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) 
{
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}


// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];

// Screen object
static SSD1306_t SSD1306;

/* Initialize the oled screen */
void ssd1306_Init(void) 
{
    // Wait for the screen to boot
    HAL_Delay(100);

    // Init OLED
    ssd1306_SetDisplayOn(0); //display off
    ssd1306_WriteCommand(0x20); //Set Memory Addressing Mode
    ssd1306_WriteCommand(0x00); // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                // 10b,Page Addressing Mode (RESET); 11b,Invalid
    ssd1306_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
    ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction / ssd1306_WriteCommand(0xC0); // Mirror vertically

    ssd1306_WriteCommand(0x00); //---set low column address
    ssd1306_WriteCommand(0x10); //---set high column address
    ssd1306_WriteCommand(0x40); //--set start line address - CHECK
    ssd1306_SetContrast(0xFF);
    ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127 - CHECK / ssd1306_WriteCommand(0xA0); // Mirror horizontally
    ssd1306_WriteCommand(0xA6); //--set normal color / ssd1306_WriteCommand(0xA7); //--set inverse color
    ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64) - CHECK / ssd1306_WriteCommand(0xFF); // if SSD1306_HEIGHT == 128
    ssd1306_WriteCommand(0x3F); // SSD1306_HEIGHT == 64 ; ssd1306_WriteCommand(0x1F); // 32 ; ssd1306_WriteCommand(0x3F); // 128
    ssd1306_WriteCommand(0xA4); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    ssd1306_WriteCommand(0xD3); //-set display offset - CHECK
    ssd1306_WriteCommand(0x00); //-not offset
    ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); //--set divide ratio
    ssd1306_WriteCommand(0xD9); //--set pre-charge period
    ssd1306_WriteCommand(0x22); //
    ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration - CHECK
    ssd1306_WriteCommand(0x12); // HEIGHT = 64; 0x12 = 128; 0x02 = 32
    ssd1306_WriteCommand(0xDB); //--set vcomh
    ssd1306_WriteCommand(0x20); //0x20,0.77xVcc
    ssd1306_WriteCommand(0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(0x14); //
    ssd1306_SetDisplayOn(1); //--turn on SSD1306 panel

    // Clear screen
    ssd1306_Fill(COLOR_BLACK);
    
    // Flush buffer to screen
    ssd1306_UpdateScreen();
    
    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    
    SSD1306.Initialized = 1;
}

/* Fill the whole screen with the given color */
void ssd1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == COLOR_BLACK) ? 0x00 : 0xFF, SSD1306_BUFFER_SIZE);
}

/* Write the screenbuffer with changed to the screen */
void ssd1306_UpdateScreen(void) {
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
    for(uint8_t i = 0; i < SSD1306_HEIGHT/8; i++) 
	{
		uint8_t cmd[3] = {0xB0 + i, 0x00 + SSD1306_X_OFFSET_LOWER, 0x10 + SSD1306_X_OFFSET_UPPER};
		HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, cmd, 3, HAL_MAX_DELAY);
//        ssd1306_WriteCommand(0xB0 + i); // Set the current RAM page address.
//        ssd1306_WriteCommand(0x00 + SSD1306_X_OFFSET_LOWER);
//        ssd1306_WriteCommand(0x10 + SSD1306_X_OFFSET_UPPER);
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH*i],SSD1306_WIDTH);
    }
}

/*
 * Draw one pixel in the screenbuffer
 * X => X Coordinate
 * Y => Y Coordinate
 * color => Pixel color
 */
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) 
	{
        // Don't write outside the buffer
        return;
    }
   
    // Draw in the right color
    if(color == COLOR_WHITE) 
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8); 
	else 
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
}

/*
 * Draw 1 char to the screen buffer
 * ch       => char om weg te schrijven
 * Font     => Font waarmee we gaan schrijven
 * color    => Black or White
 */
char ssd1306_WriteChar(char ch, SSD1306_Font_t Font, SSD1306_COLOR color) {
    uint32_t i, b, j;
    
    // Check if character is valid
    if (ch < 32 || ch > 126)
        return 0;
    
    // Check remaining space on current line
    if (SSD1306_WIDTH < (SSD1306.CurrentX + Font.width) ||
        SSD1306_HEIGHT < (SSD1306.CurrentY + Font.height))
    {
        // Not enough space on current line
        return 0;
    }
    
    // Use the font to write
    for(i = 0; i < Font.height; i++) 
	{
        b = Font.data[(ch - 32) * Font.height + i];
        for(j = 0; j < Font.width; j++) 
		{
            if((b << j) & 0x8000)  
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
			else 
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
        }
    }
    
    // The current space is now taken
    SSD1306.CurrentX += Font.char_width ? Font.char_width[ch - 32] : Font.width;
    
    // Return written char for validation
    return ch;
}

/* Write full string to screenbuffer */
char ssd1306_WriteString(char* str, SSD1306_Font_t Font, SSD1306_COLOR color) {
    while (*str) 
	{
        if (ssd1306_WriteChar(*str, Font, color) != *str)
            return *str;
        str++;
    }
    
    // Everything ok
    return *str;
}

/* Position the cursor */
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

/* Draw line by Bresenhem's algorithm */
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) 
{
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX = ((x1 < x2) ? 1 : -1);
    int32_t signY = ((y1 < y2) ? 1 : -1);
    int32_t error = deltaX - deltaY;
    int32_t error2;
    
    ssd1306_DrawPixel(x2, y2, color);

    while((x1 != x2) || (y1 != y2)) 
	{
        ssd1306_DrawPixel(x1, y1, color);
        error2 = error * 2;
        if(error2 > -deltaY) 
		{
            error -= deltaY;
            x1 += signX;
        }
        
        if(error2 < deltaX) 
		{
            error += deltaX;
            y1 += signY;
        }
    }
    return;
}

/* Draw a rectangle */
void ssd1306_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    ssd1306_Line(x1,y1,x2,y1,color);
    ssd1306_Line(x2,y1,x2,y2,color);
    ssd1306_Line(x2,y2,x1,y2,color);
    ssd1306_Line(x1,y2,x1,y1,color);

    return;
}

/* Draw a filled rectangle */
void ssd1306_FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    uint8_t x_start = ((x1<=x2) ? x1 : x2);
    uint8_t x_end   = ((x1<=x2) ? x2 : x1);
    uint8_t y_start = ((y1<=y2) ? y1 : y2);
    uint8_t y_end   = ((y1<=y2) ? y2 : y1);

    for (uint8_t y= y_start; (y<= y_end)&&(y<SSD1306_HEIGHT); y++) {
        for (uint8_t x= x_start; (x<= x_end)&&(x<SSD1306_WIDTH); x++) {
            ssd1306_DrawPixel(x, y, color);
        }
    }
    return;
}

/* Draw a bitmap */
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    for (uint8_t j = 0; j < h; j++, y++) {
        for (uint8_t i = 0; i < w; i++) {
            if (i & 7) {
                byte <<= 1;
            } else {
                byte = (*(const unsigned char *)(&bitmap[j * byteWidth + i / 8]));
            }

            if (byte & 0x80) {
                ssd1306_DrawPixel(x + i, y, color);
            }
        }
    }
    return;
}

void ssd1306_SetContrast(const uint8_t value) {
    const uint8_t kSetContrastControlRegister = 0x81;
    ssd1306_WriteCommand(kSetContrastControlRegister);
    ssd1306_WriteCommand(value);
}

void ssd1306_SetDisplayOn(const uint8_t on) {
    uint8_t value;
    if (on) {
        value = 0xAF;   // Display on
        SSD1306.DisplayOn = 1;
    } else {
        value = 0xAE;   // Display off
        SSD1306.DisplayOn = 0;
    }
    ssd1306_WriteCommand(value);
}
