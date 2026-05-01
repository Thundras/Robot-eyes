#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Eye.h>

/**
 * Robot Eyes on ESP32 with 64x128 OLED display.
 * Uses the esp32-eyes library for animated eye rendering.
 */

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// Display and eyes instances
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Eye eyes;

/**
 * Setup: Initialize hardware and eyes library.
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n[robot-eyes] Starting...");

  // Initialize I2C and OLED display
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[ERROR] Failed to initialize OLED display");
    while (1) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 28);
  display.println("Robot Eyes");
  display.display();
  delay(2000);

  // Initialize eyes on OLED display
  eyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 30, 32, 98, 32);
  eyes.setBackgroundColor(BLACK);

  Serial.println("[robot-eyes] Eyes initialized and ready");
}

/**
 * Main loop: Update eye animations and render.
 */
void loop() {
  // Clear display and update eye state
  display.clearDisplay();

  // Update eyes (handles blinking, random movements, etc.)
  eyes.update(display);

  // Render to physical display
  display.display();

  // Control frame rate
  delay(30);
}
