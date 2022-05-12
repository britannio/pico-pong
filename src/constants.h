#include <stdio.h>

// Game constants
#define GRID_SIZE 3
#define POSITIONS (GRID_SIZE * GRID_SIZE)
#define LAST_POSITION (POSITIONS - 1)

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 30

// Hardware
#define BUTTON_GPIO 21