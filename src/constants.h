#include <stdio.h>
#include "lib/st7735.h"

// Game constants
#define GRID_SIZE 3
#define POSITIONS (GRID_SIZE * GRID_SIZE)
#define LAST_POSITION (POSITIONS - 1)

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 30
#define MAX_PADDLE_Y (ST7735_WIDTH - PADDLE_HEIGHT)

#define BALL_SIZE 5

// Hardware
#define BUTTON_GPIO 21