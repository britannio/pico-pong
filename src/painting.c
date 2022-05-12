#include "painting.h"
#include "lib/st7735.h"

void paintSquare(uint16_t x, uint16_t y, uint16_t size, uint16_t color)
{
    paintVerticalLine(x, y, y + size, color);
    paintVerticalLine(x + size, y, y + size, color);
    paintHorizontalLine(y, x, x + size, color);
    paintHorizontalLine(y + size, x, x + size, color);
}

void paintVerticalLine(uint16_t x, uint16_t y1, uint16_t y2, uint16_t color)
{
    // Swap variables if necessary
    if (y1 > y2)
    {
        uint16_t temp = y1;
        y1 = y2;
        y2 = temp;
    }
    for (uint16_t y = y1; y <= y2; y++)
    {
        ST7735_DrawPixel(x, y, color);
    }
}

void paintHorizontalLine(uint16_t y, uint16_t x1, uint16_t x2, uint16_t color)
{
    // Swap variables if necessary
    if (x1 > x2)
    {
        uint16_t temp = x1;
        x1 = x2;
        x2 = temp;
    }
    for (uint16_t x = x1; x <= x2; x++)
    {
        ST7735_DrawPixel(x, y, color);
    }
}

void clearScreen()
{
  ST7735_FillScreen(ST7735_BLACK);
}