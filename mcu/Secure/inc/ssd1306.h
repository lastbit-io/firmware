#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "ssd1306_fonts.h"
#include <stdbool.h>

#define  BUFFER_LENGTH       16

// SSD1306 OLED height in pixels
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT          64
#endif

// SSD1306 width in pixels
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH           128
#endif

// some OLEDs don't display anything in first two columns
// #define SSD1306_WIDTH           130

// Enumeration for screen colors
typedef enum {
    Black = 0x00, // Black color, no pixel
    White = 0x01  // Pixel is set. Color depends on OLED
} SSD1306_COLOR;


typedef struct {
	uint8_t width, height;
	const uint8_t *data;
} BITMAP;

// Struct to store transformations
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
} SSD1306_t;

// Procedure definitions
void SSD1306_Init(void);
void SSD1306_Fill(SSD1306_COLOR color);
void SSD1306_FillMainScreen(SSD1306_COLOR color);
void SSD1306_FillTitleBar(SSD1306_COLOR color);
void SSD1306_UpdateScreen(void);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void SSD1306_InvertPixel(uint8_t x, uint8_t y);
char SSD1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color);
char SSD1306_WriteString(char* str, FontDef Font, SSD1306_COLOR color);
void SSD1306_SetCursor(uint8_t x, uint8_t y);
void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_COLOR c);
void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR c);
void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR c);
void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR c);
void SSD1306_OverlayDialog(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t ms, SSD1306_COLOR color, int num_lines, ...);
void SSD1306_Marquee_Scroll();
void SSD1306_SwipeUp();
void SSD1306_WriteStringCenter(int y, char* text, FontDef font, SSD1306_COLOR c);
void SSD1306_WriteStringTitleBar(char* text, FontDef font, SSD1306_COLOR c);
void SSD1306_WipeScreen(SSD1306_COLOR c);
void SSD1306_WipeMainScreen(SSD1306_COLOR c);
void SSD1306_WipeTitleBar(SSD1306_COLOR c);
void SSD1306_WriteStringRight(int y, char* text, FontDef font, SSD1306_COLOR c);
void SSD1306_WriteStringLeft(int y, char* text, FontDef font, SSD1306_COLOR c);
void SSD1306_DrawBitmap(int x, int y, const BITMAP *bmp, SSD1306_COLOR c);
void SSD1306_Invert(int x1, int y1, int x2, int y2);
void SSD1306_HLine(int y, SSD1306_COLOR c);
void SSD1306_ClearFrame(int x1, int y1, int x2, int y2, SSD1306_COLOR c);
void SSD1306_Frame(int x1, int y1, int x2, int y2, SSD1306_COLOR c);
void SSD1306_SwipeLeft(void);
void SSD1306_SwipeRight(void);

// Low-level procedures
void SSD1306_Reset(void);
void SSD1306_WriteCommand(uint8_t byte);
void SSD1306_WriteData(uint8_t* buffer, uint32_t buff_size);

#endif // __SSD1306_H__
