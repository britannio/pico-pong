#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "lib/fonts.h"
#include "lib/st7735.h"
#include "lib/ICM20948.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"

void paintGameOverText();
void startGame();
void restartGame();
bool repaintTask();
bool ballTask();
bool userPaddleTask();
bool aiPaddleTask();
bool accelerometerTask();
bool monitoringTask();
void paintBall();
void paintAiPaddle();
void paintUserPaddle();
void paintDivider();

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 30
#define MAX_PADDLE_Y (ST7735_WIDTH - PADDLE_HEIGHT)
#define BALL_SIZE 5
#define WATCHDOG_MILLIS 100

// Components should only be repainted if they have changed in some way.
// These flags track this.
volatile bool userPaddleDirty = true;
volatile bool aiPaddleDirty = true;

// Game state
volatile uint16_t userPaddleY = 0;
volatile uint16_t aiPaddleY = 35;
volatile uint16_t ballX = 80;
volatile uint16_t ballY = 35;
volatile uint16_t prevBallX = 80;
volatile uint16_t prevBallY = 35;
volatile int ballMagnitudeX = 1;
volatile int ballMagnitudeY = 1;
volatile bool shouldCleanReset = false;

// Timers
struct repeating_timer monitoringTimer;
struct repeating_timer repaintTimer;
struct repeating_timer ballTimer;
struct repeating_timer userPaddleTimer;
struct repeating_timer aiPaddleTimer;

int main()
{
  // INITIALISE SERIAL IN/OUTPUT
  stdio_init_all();

  // ENABLE WATCHDOG
  watchdog_enable(WATCHDOG_MILLIS, true);
  add_repeating_timer_ms(WATCHDOG_MILLIS - 10, monitoringTask, NULL, &monitoringTimer);

  // INITIALISE SCREEN (https://github.com/plaaosert/st7735-guide)
  // ---------------------------------------------------------------------------
  // Disable line and block buffering on stdout (for talking through serial)
  setvbuf(stdout, NULL, _IONBF, 0);
  // Give the Pico some time to think...
  sleep_ms(1000);
  // Initialise the screen
  ST7735_Init();
  ST7735_FillScreen(ST7735_BLACK);

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
  const int32_t tick = -16;
  add_repeating_timer_ms(tick, repaintTask, NULL, &repaintTimer);
  add_repeating_timer_ms(tick, ballTask, NULL, &ballTimer);
  add_repeating_timer_ms(tick, userPaddleTask, NULL, &userPaddleTimer);
  add_repeating_timer_ms(tick * 3, aiPaddleTask, NULL, &aiPaddleTimer);

  while (true)
    tight_loop_contents();
}

bool repaintTask()
{
  if (userPaddleDirty)
  {
    userPaddleDirty = false;
    paintUserPaddle();
  }
  if (aiPaddleDirty)
  {
    aiPaddleDirty = false;
    paintAiPaddle();
  }
  paintDivider();
  paintBall();
  return true;
}

bool ballTask()
{
  // Colission detect with edges
  if (ballY + BALL_SIZE >= ST7735_WIDTH || ballY <= 0)
  {
    // Ball is at top or bottom so change its direction
    ballMagnitudeY = -ballMagnitudeY;
  }
  bool onLeftEdge = ballX <= PADDLE_WIDTH;
  bool onRightEdge = ballX + BALL_SIZE >= ST7735_HEIGHT - PADDLE_WIDTH;
  if (onLeftEdge || onRightEdge)
  {
    if (onLeftEdge)
    {
      // Check if the ball is in range of the paddle
      bool inRange = ballY >= userPaddleY && ballY + BALL_SIZE <= userPaddleY + PADDLE_HEIGHT;
      if (!inRange)
        restartGame();
    }
    if (onRightEdge)
    {
      bool inRange = ballY >= aiPaddleY && ballY + BALL_SIZE <= aiPaddleY + PADDLE_HEIGHT;
      if (!inRange)
        restartGame();
    }

    // Ball is at left or right edge so change its direction
    ballMagnitudeX = -ballMagnitudeX;
  }

  // Save the current position so it can be efficiently erased
  prevBallX = ballX;
  prevBallY = ballY;

  const uint16_t ballStep = 1;

  ballX += ballStep * ballMagnitudeX;
  ballY += ballStep * ballMagnitudeY;

  return true;
}

bool aiPaddleTask()
{
  uint16_t paddleCenterY = aiPaddleY + (PADDLE_HEIGHT / 2);
  uint16_t ballCenterY = ballY + (BALL_SIZE / 2);
  int delta = paddleCenterY - ballCenterY;

  // The number of pixels to move
  uint16_t step = 2;

  if (delta > 0 && aiPaddleY >= step)
  {
    // Move paddle up
    aiPaddleDirty = true;
    aiPaddleY -= step;
  }
  if (delta < 0 && aiPaddleY + PADDLE_HEIGHT + step <= ST7735_WIDTH)
  {
    // Move paddle down
    aiPaddleDirty = true;
    aiPaddleY += step;
  }

  return true;
}

bool userPaddleTask()
{
  float x;
  float y;
  float z;

  // Use the addresses of our variables.
  // This function sets data at a pointer, so we give it three pointers.
  icm20948AccelRead(&x, &y, &z);

  // Down = +x
  // printf("x: %.2f\n", x);
  const float threshold = 0.3;

  const bool atTop = userPaddleY <= 0;
  const bool atBottom = userPaddleY >= MAX_PADDLE_Y;

  // The number of pixels to move if the device is tilted.
  const uint16_t step = 2;

  // Detect action
  if (x > threshold && !atBottom)
  {
    // Move down
    userPaddleDirty = true;
    userPaddleY += step;
  }
  else if (x < -threshold && !atTop)
  {
    // Move up
    userPaddleDirty = true;
    userPaddleY -= step;
  }

  return true;
}

bool monitoringTask()
{
  if (shouldCleanReset)
  {
    // Causes the watchdog to reset
    sleep_ms(WATCHDOG_MILLIS);
  }
  watchdog_update();
  return true;
}

void restartGame()
{
  cancel_repeating_timer(&repaintTimer);
  ST7735_FillScreen(ST7735_RED);
  shouldCleanReset = true;
}

void paintUserPaddle()
{
  // Clear area above paddle
  ST7735_FillRectangle(
      ST7735_WIDTH - userPaddleY,
      0,
      userPaddleY,
      PADDLE_WIDTH,
      ST7735_BLACK);
  // Clear area below paddle
  ST7735_FillRectangle(
      0,
      0,
      ST7735_WIDTH - userPaddleY - ST7735_WIDTH,
      PADDLE_WIDTH,
      ST7735_BLACK);
  // Paint user paddle
  ST7735_FillRectangle(
      ST7735_WIDTH - PADDLE_HEIGHT - userPaddleY,
      0,
      PADDLE_HEIGHT,
      PADDLE_WIDTH,
      ST7735_YELLOW);
}

void paintAiPaddle()
{
  // Clear paddle area
  const uint16_t y = ST7735_HEIGHT - PADDLE_WIDTH;
  ST7735_FillRectangle(0, y, ST7735_WIDTH, PADDLE_WIDTH, ST7735_BLACK);
  // paint ai paddle
  ST7735_FillRectangle(
      ST7735_WIDTH - PADDLE_HEIGHT - aiPaddleY,
      ST7735_HEIGHT - PADDLE_WIDTH,
      PADDLE_HEIGHT,
      PADDLE_WIDTH,
      ST7735_YELLOW);
}

void paintBall()
{
  // Clear previous ball position
  ST7735_FillRectangle(
      ST7735_WIDTH - BALL_SIZE - prevBallY,
      prevBallX,
      BALL_SIZE,
      BALL_SIZE,
      ST7735_BLACK);
  // Paint ball
  ST7735_FillRectangle(
      ST7735_WIDTH - BALL_SIZE - ballY,
      ballX,
      BALL_SIZE,
      BALL_SIZE,
      ST7735_GREEN);
}

void paintDivider()
{
  // Line to split the screen
  for (uint16_t x = 0; x <= ST7735_WIDTH; x++)
  {
    ST7735_DrawPixel(x, ST7735_HEIGHT / 2, ST7735_WHITE);
  }
}
