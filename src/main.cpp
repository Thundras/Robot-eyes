#include <Arduino.h>
#include <U8g2lib.h>
#include <Eye.h>

/**
 * Robot Eyes on ESP32 with 64x128 OLED display.
 * Uses the esp32-eyes library with U8g2 rendering.
 */

// U8g2 OLED display instance (SSD1306, I2C mode)
// SDA=GPIO21, SCL=GPIO22
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

// Face instance for eyes library
Face face;

// Timing for frame-rate-independent animation
static unsigned long lastUpdateTime = 0;

/**
 * Setup: Initialize hardware and display.
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n[robot-eyes] Starting...");

  // Initialize U8G2 display
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(20, 32, "Robot Eyes");
  u8g2.sendBuffer();
  delay(2000);

  // Initialize face for eyes library
  face.begin(u8g2);

  Serial.println("[robot-eyes] Eyes initialized and ready");
}

/**
 * Main loop: Update eye animations and render.
 */
void loop() {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
  lastUpdateTime = currentTime;

  // Clear display and update face/eyes
  u8g2.clearBuffer();

  // Update face (handles eye animation)
  face.Update();

  // Render to physical display
  u8g2.sendBuffer();
}
