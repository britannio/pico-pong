#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "lib/fonts.h"
#include "lib/st7735.h"
#include "painting.h"
#include "logic.h"
#include "constants.h"
#include "lib/ICM20948.h"
#include "pico/multicore.h"

typedef enum
{
  Left,
  Right,
  Up,
  Down
} Move;

void core1_entry();
void paintGrid();
void paintCursor();
void paintHuman(uint8_t pos);
void paintAI(uint8_t pos);
void buttonCallback(uint gpio, uint32_t events);
void updatePosWithMove(Move move);
void paintGameOverText();
void startGame();

bool repaintTask();
bool ballTask();
bool userPaddleTask();
bool aiPaddleTask();

static int cursorPos = 0;

// Initialise the grid
GridPos grid[POSITIONS] =
    {[0 ... LAST_POSITION] = (GridPos){.player = empty, .winningPos = false}};

// The second core (core 1) reads accelerometer data, converts it to
// (left/right/up/down) then puts it onto the multicore queue where the first
// core can receive it.
void core1_entry()
{
  float x;
  float y;
  float z;
  printf("Running core1_entry()\n");

  while (true)
  {
    sleep_ms(500);
    // Use the addresses of our variables.
    // This function sets data at a pointer, so we give it three pointers.
    icm20948AccelRead(&x, &y, &z);
    // Left = +x
    // Up = +y
    // printf("x\ty\n%.2f\t%.2f\n", x, y);

    const float xThreshold = 0.6;
    const float yThreshold = 0.4;
    Move move = -1;

    // Detect action
    if (x > xThreshold)
      move = Left;
    else if (x < -xThreshold)
      move = Right;
    else if (y > yThreshold)
      move = Up;
    else if (y < -yThreshold)
      move = Down;

    if (move == -1)
    {
      continue;
    }

    multicore_fifo_push_blocking(move);
  }
}

int main()
{
  // INITIALISE SERIAL IN/OUTPUT
  stdio_init_all();

  // INITIALISE SCREEN (https://github.com/plaaosert/st7735-guide)
  // ---------------------------------------------------------------------------
  // Disable line and block buffering on stdout (for talking through serial)
  setvbuf(stdout, NULL, _IONBF, 0);
  // Give the Pico some time to think...
  sleep_ms(1000);
  // Initialise the screen
  ST7735_Init();
  clearScreen();

  // INITIALISE ACCELEROMETER (https://github.com/plaaosert/icm20948-guide)
  // ---------------------------------------------------------------------------
  i2c_init(i2c0, 400 * 1000);
  gpio_set_function(4, GPIO_FUNC_I2C);
  gpio_set_function(5, GPIO_FUNC_I2C);
  gpio_pull_up(4);
  gpio_pull_up(5);
  IMU_EN_SENSOR_TYPE enMotionSensorType;
  imuInit(&enMotionSensorType);
  if (IMU_EN_SENSOR_TYPE_ICM20948 != enMotionSensorType)
  {
    printf("Failed to initialise IMU...\n");
  }
  printf("IMU initialised!\n");

  startGame();
}

void startGame()
{
  printf("Starting game\n");

  // Timers
  // ---------------------------------------------------------------------------
  struct repeating_timer repaintTimer;
  add_repeating_timer_ms(-16, repaintTask, NULL, &repaintTimer);
  struct repeating_timer ballTimer;
  add_repeating_timer_ms(-16, ballTask, NULL, &ballTimer);
  struct repeating_timer userPaddleTimer;
  add_repeating_timer_ms(-16, userPaddleTask, NULL, &userPaddleTimer);
  struct repeating_timer aiPaddleTimer;
  add_repeating_timer_ms(-32, aiPaddleTask, NULL, &aiPaddleTimer);

  while (true) tight_loop_contents();
}

bool repaintTask()
{
  clearScreen();
  // Paint user paddle
  // paint ai paddle
  // paint ball
  // paint vertical line to split screen
  // ---

  // Line to split the screen
  paintHorizontalLine(ST7735_HEIGHT / 2, 0, ST7735_WIDTH, ST7735_WHITE);

  return true;
}

bool ballTask()
{
  return true;
}

bool userPaddleTask()
{
  return true;
}

bool aiPaddleTask()
{
  return true;
}

// This method is invoked when the button is pressed.
// It does the following:
// 1. Uses the current cursor position to attempt to play a nought.
// 2. If successful, it repaints the grid then the AI responds with it's own move.
// 3. If the AI was able to play a move, the grid is repainted once more.
void buttonCallback(uint gpio, uint32_t events)
{
  printf("GPIO %d PRESSED\n", BUTTON_GPIO);
  // TODO implement buttonCallback()
}



void updatePosWithMove(Move move)
{
  switch (move)
  {
  case Left:
    if (cursorPos % GRID_SIZE == 0)
    {
      // Cursor is on the left edge.
      printf("Rejecting move Left: %d\n", cursorPos);
      return;
    }
    printf("Moving Left\n");
    cursorPos--;
    break;
  case Right:
    if ((cursorPos + 1) % GRID_SIZE == 0)
    {
      // Cursor is on the right edge.
      printf("Rejecting move Right: %d\n", cursorPos);
      return;
    }
    printf("Moving Right\n");
    cursorPos++;
    break;
  case Up:
    if (cursorPos < GRID_SIZE)
    {
      // Cursor is on the top edge.
      printf("Rejecting move Up: %d\n", cursorPos);
      return;
    }
    printf("Moving Up\n");
    cursorPos -= 3;
    break;
  case Down:
    if (cursorPos >= POSITIONS - GRID_SIZE)
    {
      // Cursor is on the bottom edge.
      printf("Rejecting move Down: %d\n", cursorPos);
      return;
    }
    printf("Moving Down\n");
    cursorPos += 3;
    break;
  }
}

void paintGrid()
{
  // Clear top half of screen
  ST7735_FillRectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT / 2, ST7735_BLACK);

  // Paint the lines forming the grid
  const uint16_t oneThird = ST7735_WIDTH / 3;
  const uint16_t twoThirds = oneThird * 2;
  paintVerticalLine(oneThird, 0, ST7735_WIDTH, ST7735_WHITE);
  paintVerticalLine(twoThirds, 0, ST7735_WIDTH, ST7735_WHITE);
  paintHorizontalLine(oneThird, 0, ST7735_WIDTH, ST7735_WHITE);
  paintHorizontalLine(twoThirds, 0, ST7735_WIDTH, ST7735_WHITE);

  // Paint the players
  for (int i = 0; i < POSITIONS; i++)
  {
    switch (grid[i].player)
    {
    case empty:
      break;
    case human:
      paintHuman(i);
      break;
    case ai:
      paintAI(i);
      break;
    }
  }

  paintCursor();
}

void paintCursor()
{
  // Roughly one third of the display.
  const uint16_t oneThird = 26;

  // Top left of the cell
  uint16_t x = cursorPos % 3;
  x += x * oneThird;
  uint16_t y = cursorPos / 3;
  y += y * oneThird;

  // Add inset
  const uint16_t inset = 12; // Just under half of the box
  x += inset;
  y += inset;
  uint16_t size = 4;
  ST7735_FillRectangle(x, y, size, size, ST7735_GREEN);
}

void paintHuman(uint8_t pos)
{
  const uint16_t oneThird = 26;

  // Paint a square
  uint16_t x = (pos % 3);
  x += x * oneThird;
  uint16_t y = pos / 3;
  y += y * oneThird;

  // Add inset
  const uint16_t inset = 5;
  x += inset;
  y += inset;
  paintSquare(x, y, oneThird - (2 * inset), ST7735_WHITE);
}

void paintAI(uint8_t pos)
{
  const uint16_t oneThird = 26;

  uint16_t x = (pos % 3);
  x += x * oneThird;
  uint16_t y = pos / 3;
  y += y * oneThird;

  const uint16_t inset = 5;
  paintVerticalLine(x + (oneThird / 2), y + inset, y + oneThird - inset, ST7735_WHITE);
  paintHorizontalLine(y + (oneThird / 2), x + inset, x + oneThird - inset, ST7735_WHITE);
}

void paintGameOverText()
{
  const uint16_t textHeight = 26;
  const uint16_t y = 94;
  const uint16_t inset = 8;
  ST7735_WriteString(inset, y, "GAME", Font_16x26, ST7735_RED, ST7735_BLACK);
  ST7735_WriteString(inset, y + textHeight, "OVER", Font_16x26, ST7735_RED, ST7735_BLACK);
}