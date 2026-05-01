#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>

/**
 * Display controller for 64x128 OLED.
 * Manages initialization, drawing, and animations.
 */
class Display {
public:
  Display();
  bool begin();
  void clear();
  void drawBitmap(int x, int y, const uint8_t *bitmap, int width, int height);
  void display();

private:
  Adafruit_SSD1306 oled;
  static const int SCREEN_WIDTH = 128;
  static const int SCREEN_HEIGHT = 64;
  static const int I2C_ADDR = 0x3C;
};

#endif
