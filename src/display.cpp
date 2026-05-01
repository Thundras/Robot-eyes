#include "display.h"

// SSD1306 I2C address
#define SCREEN_I2C_ADDR 0x3C

// I2C pins
#define I2C_SDA 21
#define I2C_SCL 22

/**
 * Initialize display controller.
 */
Display::Display() : oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {}

/**
 * Begin OLED display initialization.
 * <returns>true if successful, false otherwise</returns>
 */
bool Display::begin() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDR)) {
    return false;
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 0);
  oled.println("Robot Eyes");
  oled.display();

  return true;
}

/**
 * Clear display buffer.
 */
void Display::clear() {
  oled.clearDisplay();
}

/**
 * Draw bitmap at specified coordinates.
 * <param name="x">X coordinate</param>
 * <param name="y">Y coordinate</param>
 * <param name="bitmap">Bitmap data</param>
 * <param name="width">Bitmap width</param>
 * <param name="height">Bitmap height</param>
 */
void Display::drawBitmap(int x, int y, const uint8_t *bitmap, int width, int height) {
  oled.drawBitmap(x, y, bitmap, width, height, WHITE);
}

/**
 * Update physical display with buffer contents.
 */
void Display::display() {
  oled.display();
}
