#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "NuMicro.h"
#include "ssd1306.h"
#include "defines.h"

extern float GET_Battery_Level();

void SSD1306_Reset(void)
{
	// Reset the OLED
	PC7 = 0;
	CLK_SysTickLongDelay(10000);
	PC7 = 1;
	CLK_SysTickLongDelay(10000);
}

// Send a byte to the command register
void SSD1306_WriteCommand(uint8_t byte)
{
	PA0 = 0;
	//printf("[%s]-%d - %x\r\n", __func__, __LINE__, byte&0xFF);
	SPI_WRITE_TX(SPI0, byte);
	while (SPI_IS_BUSY(SPI0))
		;
}

// Send data
void SSD1306_WriteData(uint8_t *buffer, uint32_t buff_size)
{
	PA0 = 1;
	//printf("[%s]-%d - %d\r\n", __func__, __LINE__, buff_size);
	for (int i = 0; i < buff_size; i++)
	{
		SPI_WRITE_TX(SPI0, buffer[i]);
		while (SPI_IS_BUSY(SPI0))
			;
	}
}

// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Screen object
static SSD1306_t SSD1306;

// Initialize the oled screen
void SSD1306_Init(void)
{
	// Reset OLED
	printf("[%s]-%d\r\n", __func__, __LINE__);
	SSD1306_Reset();

	// Wait for the screen to boot
	CLK_SysTickLongDelay(100000);

	// Init OLED
	SSD1306_WriteCommand(0xAE); //display off

	SSD1306_WriteCommand(0xD5); //display clock divider
	SSD1306_WriteCommand(0x80); // the suggested ratio 0x80
	SSD1306_WriteCommand(0xA8); //set multiplex
	SSD1306_WriteCommand(63);	//lcdheight-1

	SSD1306_WriteCommand(0xD3); //display offset
	SSD1306_WriteCommand(0x0);	//no offset

	SSD1306_WriteCommand(0x40 | 0x0); // set start line #0

	SSD1306_WriteCommand(0x8D); // SSD1306_CHARGEPUMP

	SSD1306_WriteCommand(0x14);

	SSD1306_WriteCommand(0x20); //Set Memory Addressing Mode
	SSD1306_WriteCommand(0x00); // 00,Horizontal Addressing Mode; 01,Vertical Addressing Mode;
								// 10,Page Addressing Mode (RESET); 11,Invalid

	SSD1306_WriteCommand(0xA0); //SSD1306_SEGREMAP - 0xA0 | 0x1 for default rotation
	SSD1306_WriteCommand(0xC0); //Set COM Output Scan Direction //COMSCANDEC 0xC8 for default direction

	SSD1306_WriteCommand(0xDA); //SSD1306_SETCOMPINS
	SSD1306_WriteCommand(0x12);
	SSD1306_WriteCommand(0x81); //SSD1306_SETCONTRAST
	SSD1306_WriteCommand(0xCF);

	SSD1306_WriteCommand(0xd9); //SSD1306_SETPRECHARGE
	SSD1306_WriteCommand(0xF1);
	SSD1306_WriteCommand(0xDB); //SSD1306_SETVCOMDETECT
	SSD1306_WriteCommand(0x40);
	SSD1306_WriteCommand(0xA4); //SSD1306_DISPLAYALLON_RESUME
	SSD1306_WriteCommand(0xA6); //SSD1306_NORMALDISPLAY

	SSD1306_WriteCommand(0x2E); //SSD1306_DEACTIVATE_SCROLL
	SSD1306_WriteCommand(0xAF); //SSD1306_DISPLAYON

	// Set default values for screen object
	SSD1306.CurrentX = 0;
	SSD1306.CurrentY = 0;

	SSD1306.Initialized = 1;
}

void SSD1306_OverlayDialog(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t ms, SSD1306_COLOR color, int num_lines, ...)
{

	uint8_t temp_buffer[SSD1306_HEIGHT * SSD1306_WIDTH / 8];
	for (int i = 0; i < SSD1306_HEIGHT * SSD1306_WIDTH / 8; i++)
		temp_buffer[i] = SSD1306_Buffer[i];
	SSD1306_WipeMainScreen(!color);
	SSD1306_Frame(x1, y1, x2, y2, color);

	va_list text_lines;
	va_start(text_lines, num_lines);
	int offset = 0;
	for (int i = 0; i < num_lines; i++)
	{
		SSD1306_WriteStringCenter((y2 - y1) / 2 + offset, va_arg(text_lines, char *), Font_7x10, color);
		offset += 12;
	}
	va_end(text_lines);
	SSD1306_UpdateScreen();
	CLK_SysTickLongDelay(ms * 1000);
	for (int i = 0; i < SSD1306_HEIGHT * SSD1306_WIDTH / 8; i++)
		SSD1306_Buffer[i] = temp_buffer[i];
	SSD1306_UpdateScreen();
}

void SSD1306_Marquee_Scroll()
{
	SSD1306_WriteCommand(0x26); // RIGHT_HORIZONTAL_SCROLL
	SSD1306_WriteCommand(0x00); // DUMMY BYTE
	SSD1306_WriteCommand(0x09); // START PAGE
	SSD1306_WriteCommand(0xFF); // SCROLL SPEED
	SSD1306_WriteCommand(0x0A); // STOP PAGE
	SSD1306_WriteCommand(0x00);
	SSD1306_WriteCommand(0xFF);
	SSD1306_WriteCommand(0x2F); // ACTIVATE SCROLL
								/*
    SSD1306_WriteCommand(0x29); // VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL
    SSD1306_WriteCommand(0x00); // DUMMY BYTE
    SSD1306_WriteCommand(0x00); // START PAG
    SSD1306_WriteCommand(0x00); 
    SSD1306_WriteCommand(0x07);
    SSD1306_WriteCommand(0x01);
    SSD1306_WriteCommand(0x2F);	// ACTIVATE SCROLL
    */
}

// Fill the whole screen with the given color
void SSD1306_Fill(SSD1306_COLOR color)
{
	/* Set memory */
	uint32_t i;

	for (i = 0; i < sizeof(SSD1306_Buffer); i++)
	{
		SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
	}
}

// Fill the main screen with the given color
void SSD1306_FillMainScreen(SSD1306_COLOR color)
{
	/* Set memory */
	uint32_t i;

	for (i = 130; i < sizeof(SSD1306_Buffer); i++)
	{
		SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
	}
}

// Fill the title bar with the given color
void SSD1306_FillTitleBar(SSD1306_COLOR color)
{
	/* Set memory */
	uint32_t i;

	for (i = 0; i < 131; i++)
	{
		SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
	}
}

// Write the screenbuffer with changes to the screen
void SSD1306_UpdateScreen(void)
{
	uint8_t i;
	GET_Battery_Level();
	for (i = 0; i < 8; i++)
	{
		SSD1306_WriteCommand(0xB0 + i);
		SSD1306_WriteCommand(0x00);
		SSD1306_WriteCommand(0x10);
		SSD1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
	}
}

//    Draw one pixel in the screenbuffer
//    X => X Coordinate
//    Y => Y Coordinate
//    color => Pixel color
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
	//x = SSD1306_WIDTH - x;
	//y = SSD1306_HEIGHT - y;
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
	{
		// Don't write outside the buffer
		return;
	}

	// Check if pixel should be inverted
	if (SSD1306.Inverted)
	{
		color = (SSD1306_COLOR)!color;
	}

	// Draw in the right color
	if (color == White)
	{
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
	}
	else
	{
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
	}
}

void SSD1306_InvertPixel(uint8_t x, uint8_t y)
{
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
	{
		// Don't write outside the buffer
		return;
	}
	SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] ^= 1 << (y % 8);
}

char SSD1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color)
{
	uint32_t i, b, j;

	// Check remaining space on current line
	if (SSD1306_WIDTH <= (SSD1306.CurrentX + Font.FontWidth) ||
		SSD1306_HEIGHT <= (SSD1306.CurrentY + Font.FontHeight))
	{
		// Not enough space on current line, go to next line
		SSD1306.CurrentX = 0;
		SSD1306.CurrentY += Font.FontHeight;
	}

	// Use the font to write
	for (i = 0; i < Font.FontHeight; i++)
	{
		b = Font.data[(ch - 32) * Font.FontHeight + i];
		for (j = 0; j < Font.FontWidth; j++)
		{
			if ((b << j) & 0x8000)
			{
				SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)color);
			}
			else
			{
				SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
			}
		}
	}

	// The current space is now taken
	SSD1306.CurrentX += Font.FontWidth;

	// Return written char for validation
	return ch;
}

// Write full string to screenbuffer
//TODO: Parse string for newline and setcursor

char SSD1306_WriteString(char *str, FontDef Font, SSD1306_COLOR color)
{
	// Write until null-byte
	while (*str)
	{
		if (SSD1306_WriteChar(*str, Font, color) != *str)
		{
			// Char could not be written
			return *str;
		}

		// Next char
		str++;
	}

	// Everything ok
	return *str;
}

// Position the cursor
void SSD1306_SetCursor(uint8_t x, uint8_t y)
{
	SSD1306.CurrentX = x;
	SSD1306.CurrentY = y;
}

void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_COLOR c)
{
	int16_t dx, dy, sx, sy, err, e2, i, tmp;

	/* Check for overflow */
	if (x0 >= SSD1306_WIDTH)
	{
		x0 = SSD1306_WIDTH - 1;
	}
	if (x1 >= SSD1306_WIDTH)
	{
		x1 = SSD1306_WIDTH - 1;
	}
	if (y0 >= SSD1306_HEIGHT)
	{
		y0 = SSD1306_HEIGHT - 1;
	}
	if (y1 >= SSD1306_HEIGHT)
	{
		y1 = SSD1306_HEIGHT - 1;
	}

	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
	sx = (x0 < x1) ? 1 : -1;
	sy = (y0 < y1) ? 1 : -1;
	err = ((dx > dy) ? dx : -dy) / 2;

	if (dx == 0)
	{
		if (y1 < y0)
		{
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}

		if (x1 < x0)
		{
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}

		/* Vertical line */
		for (i = y0; i <= y1; i++)
		{
			SSD1306_DrawPixel(x0, i, c);
		}

		/* Return from function */
		return;
	}

	if (dy == 0)
	{
		if (y1 < y0)
		{
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}

		if (x1 < x0)
		{
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}

		/* Horizontal line */
		for (i = x0; i <= x1; i++)
		{
			SSD1306_DrawPixel(i, y0, c);
		}

		/* Return from function */
		return;
	}

	while (1)
	{
		SSD1306_DrawPixel(x0, y0, c);
		if (x0 == x1 && y0 == y1)
		{
			break;
		}
		e2 = err;
		if (e2 > -dx)
		{
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy)
		{
			err += dx;
			y0 += sy;
		}
	}
}

void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR c)
{
	uint8_t i;

	/* Check input parameters */
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT)
	{
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH)
	{
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT)
	{
		h = SSD1306_HEIGHT - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++)
	{
		/* Draw lines */
		SSD1306_DrawLine(x, y + i, x + w, y + i, c);
	}
}

void SSD1306_WipeScreen(SSD1306_COLOR c)
{
	SSD1306_Fill(c);
	SSD1306_UpdateScreen();
}

void SSD1306_WipeMainScreen(SSD1306_COLOR c)
{
	SSD1306_FillMainScreen(c);
	SSD1306_UpdateScreen();
}
void SSD1306_WipeTitleBar(SSD1306_COLOR c)
{
	SSD1306_FillTitleBar(c);
	SSD1306_UpdateScreen();
}

void SSD1306_WriteStringTitleBar(char *text, FontDef font, SSD1306_COLOR c)
{
	SSD1306_SetCursor(SSD1306_WIDTH / 2 - strlen(text) / 2 * font.FontWidth - 2, 0);
	SSD1306_WriteString(text, font, c);
	SSD1306_UpdateScreen();
}

void SSD1306_WriteStringCenter(int y, char *text, FontDef font, SSD1306_COLOR c)
{
	SSD1306_SetCursor(SSD1306_WIDTH / 2 - strlen(text) / 2 * font.FontWidth, y);
	SSD1306_WriteString(text, font, c);
}

void SSD1306_WriteStringRight(int y, char *text, FontDef font, SSD1306_COLOR c)
{
	SSD1306_SetCursor(SSD1306_WIDTH / 2, y);
	SSD1306_WriteString(text, font, c);
}

void SSD1306_WriteStringLeft(int y, char *text, FontDef font, SSD1306_COLOR c)
{
	SSD1306_SetCursor(2, y);
	SSD1306_WriteString(text, font, c);
}

void SSD1306_DrawBitmap(int x, int y, const BITMAP *bmp, SSD1306_COLOR c)
{
	for (int i = 0; i < bmp->width; i++)
	{
		for (int j = 0; j < bmp->height; j++)
		{
			if (bmp->data[(i / 8) + j * bmp->width / 8] & (1 << (7 - i % 8)))
			{
				SSD1306_DrawPixel(x + i, y + j, c);
			}
			else
			{
				SSD1306_DrawPixel(x + i, y + j, (SSD1306_COLOR)!c);
			}
		}
	}
}

/*
 * Inverts box between (x1,y1) and (x2,y2) inclusive.
 */
void SSD1306_Invert(int x1, int y1, int x2, int y2)
{
	x1 = MAX(x1, 0);
	y1 = MAX(y1, 0);
	x2 = MIN(x2, SSD1306_WIDTH - 1);
	y2 = MIN(y2, SSD1306_HEIGHT - 1);
	for (int x = x1; x <= x2; x++)
	{
		for (int y = y1; y <= y2; y++)
		{
			SSD1306_InvertPixel(x, y);
		}
	}
}

void SSD1306_HLine(int y, SSD1306_COLOR c)
{
	if (y < 0 || y >= SSD1306_HEIGHT)
	{
		return;
	}
	for (int x = 0; x < SSD1306_WIDTH; x++)
	{
		SSD1306_DrawPixel(x, y, c);
	}
}

/*
 * Draw a rectangle frame.
 */
void SSD1306_Frame(int x1, int y1, int x2, int y2, SSD1306_COLOR c)
{
	for (int x = x1; x <= x2; x++)
	{
		SSD1306_DrawPixel(x, y1, c);
		SSD1306_DrawPixel(x, y2, c);
	}
	for (int y = y1 + 1; y < y2; y++)
	{
		SSD1306_DrawPixel(x1, y, c);
		SSD1306_DrawPixel(x2, y, c);
	}
}

/*
 * Clear a rectangle frame.
 */
void SSD1306_ClearFrame(int x1, int y1, int x2, int y2, SSD1306_COLOR c)
{
	for (int x = x1; x <= x2; x++)
	{
		SSD1306_DrawPixel(x, y1, (SSD1306_COLOR)!c);
		SSD1306_DrawPixel(x, y2, (SSD1306_COLOR)!c);
	}
	for (int y = y1 + 1; y < y2; y++)
	{
		SSD1306_DrawPixel(x1, y, (SSD1306_COLOR)!c);
		SSD1306_DrawPixel(x2, y, (SSD1306_COLOR)!c);
	}
}

void SSD1306_SwipeRight(void)
{
	for (int i = 0; i < SSD1306_WIDTH / 4; i++)
	{
		for (int j = 2; j < SSD1306_HEIGHT / 8; j++)
		{
			for (int k = SSD1306_WIDTH / 4 - 1; k > 0; k--)
			{
				SSD1306_Buffer[k * 4 - 0 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 - 4 + j * SSD1306_WIDTH];
				SSD1306_Buffer[k * 4 - 1 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 - 5 + j * SSD1306_WIDTH];
				SSD1306_Buffer[k * 4 - 2 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 - 6 + j * SSD1306_WIDTH];
				SSD1306_Buffer[k * 4 - 3 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 - 7 + j * SSD1306_WIDTH];
			}
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 1] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 2] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 3] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 4] = 0;
		}
		SSD1306_UpdateScreen();
	}
}

void SSD1306_SwipeUp(void)
{
	for (int i = 0; i < SSD1306_WIDTH / 4; i++)
	{
		for (int j = 2; j < SSD1306_HEIGHT / 8; j++)
		{
			for (int k = SSD1306_WIDTH / 4 - 1; k > 0; k--)
			{
				SSD1306_Buffer[j * SSD1306_WIDTH - k * 4 + 4] = SSD1306_Buffer[j * SSD1306_WIDTH + k * 4 - 0];
				SSD1306_Buffer[j * SSD1306_WIDTH - k * 4 + 5] = SSD1306_Buffer[j * SSD1306_WIDTH + k * 4 - 1];
				SSD1306_Buffer[j * SSD1306_WIDTH - k * 4 + 6] = SSD1306_Buffer[j * SSD1306_WIDTH + k * 4 - 2];
				SSD1306_Buffer[j * SSD1306_WIDTH - k * 4 + 7] = SSD1306_Buffer[j * SSD1306_WIDTH + k * 4 - 3];
			}
			SSD1306_Buffer[j * SSD1306_WIDTH - SSD1306_WIDTH - 1] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH - SSD1306_WIDTH - 2] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH - SSD1306_WIDTH - 3] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH - SSD1306_WIDTH - 4] = 0;
		}
		SSD1306_UpdateScreen();
	}
}

void SSD1306_SwipeLeft(void)
{
	for (int i = 0; i < SSD1306_WIDTH / 4; i++)
	{
		for (int j = 2; j < SSD1306_HEIGHT / 8; j++)
		{
			for (int k = 0; k < SSD1306_WIDTH / 4 - 1; k++)
			{
				SSD1306_Buffer[k * 4 + 0 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 + 4 + j * SSD1306_WIDTH];
				SSD1306_Buffer[k * 4 + 1 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 + 5 + j * SSD1306_WIDTH];
				SSD1306_Buffer[k * 4 + 2 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 + 6 + j * SSD1306_WIDTH];
				SSD1306_Buffer[k * 4 + 3 + j * SSD1306_WIDTH] = SSD1306_Buffer[k * 4 + 7 + j * SSD1306_WIDTH];
			}
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 1] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 2] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 3] = 0;
			SSD1306_Buffer[j * SSD1306_WIDTH + SSD1306_WIDTH - 4] = 0;
		}
		SSD1306_UpdateScreen();
	}
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR c)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	SSD1306_DrawPixel(x0, y0 + r, c);
	SSD1306_DrawPixel(x0, y0 - r, c);
	SSD1306_DrawPixel(x0 + r, y0, c);
	SSD1306_DrawPixel(x0 - r, y0, c);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		SSD1306_DrawPixel(x0 + x, y0 + y, c);
		SSD1306_DrawPixel(x0 - x, y0 + y, c);
		SSD1306_DrawPixel(x0 + x, y0 - y, c);
		SSD1306_DrawPixel(x0 - x, y0 - y, c);

		SSD1306_DrawPixel(x0 + y, y0 + x, c);
		SSD1306_DrawPixel(x0 - y, y0 + x, c);
		SSD1306_DrawPixel(x0 + y, y0 - x, c);
		SSD1306_DrawPixel(x0 - y, y0 - x, c);
	}
}

void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR c)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	SSD1306_DrawPixel(x0, y0 + r, c);
	SSD1306_DrawPixel(x0, y0 - r, c);
	SSD1306_DrawPixel(x0 + r, y0, c);
	SSD1306_DrawPixel(x0 - r, y0, c);
	SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, c);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
		SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

		SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
		SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
	}
}
