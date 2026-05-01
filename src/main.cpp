#include <Arduino.h>
#include "display.h"

// Global display instance
Display display;

/**
 * Setup: Initialize hardware and display.
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n[robot-eyes] Starting...");

  if (!display.begin()) {
    Serial.println("[ERROR] Failed to initialize OLED display");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("[robot-eyes] OLED display initialized");
}

/**
 * Main loop: Update display and animations.
 */
void loop() {
  // Clear display
  display.clear();

  // Draw placeholder eyes (simple circles)
  display.drawBitmap(20, 16, nullptr, 24, 24);
  display.drawBitmap(84, 16, nullptr, 24, 24);

  // Update display
  display.display();

  delay(100);
}
