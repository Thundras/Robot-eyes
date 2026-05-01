#include <Arduino.h>
#include <U8g2lib.h>

/**
 * Robot Eyes on ESP32 with 64x128 OLED display.
 * Animated eyes with blinking and random movement using U8g2 rendering.
 */

// U8g2 OLED display (SSD1306, I2C on GPIO21/22)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, 22, 21);

// Eye parameters
struct Eye {
  int x, y;
  int baseX, baseY;
  int offsetX, offsetY;
  int targetOffsetX, targetOffsetY;
  int width, height;
};

Eye leftEye = {45, 32, 45, 32, 0, 0, 0, 0, 20, 24};
Eye rightEye = {80, 32, 80, 32, 0, 0, 0, 0, 20, 24};

// Animation state
struct Animation {
  unsigned long lastBlinkTime;
  unsigned long lastMoveTime;
  int blinkState;
  int moveSpeed;
  int blinkDuration;
  int blinkInterval;
};

Animation anim = {0, 0, 0, 3, 150, 4000};

// Timing
static unsigned long lastUpdateTime = 0;

/**
 * Draw a single eye with iris and pupil.
 */
void drawEye(const Eye& eye) {
  int eyeX = eye.baseX + eye.offsetX;
  int eyeY = eye.baseY + eye.offsetY;

  display.setDrawColor(1);

  // Eye white (filled rounded rect)
  display.drawRFrame(eyeX, eyeY, eye.width, eye.height, 3);

  // Iris (circle)
  int irisX = eyeX + eye.width / 2;
  int irisY = eyeY + eye.height / 2;
  display.drawCircle(irisX, irisY, 5);

  // Pupil (small filled circle)
  display.drawDisc(irisX, irisY, 2);
}

/**
 * Update eye position with smooth interpolation.
 */
void updateEyePosition(Eye& eye) {
  eye.offsetX += (eye.targetOffsetX - eye.offsetX) / anim.moveSpeed;
  eye.offsetY += (eye.targetOffsetY - eye.offsetY) / anim.moveSpeed;
}

/**
 * Setup: Initialize hardware and display.
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n[robot-eyes] Starting...");

  // Initialize display
  display.begin();
  display.setFont(u8g2_font_ncenB08_tr);
  display.clearBuffer();
  display.drawStr(30, 32, "Robot Eyes");
  display.sendBuffer();
  delay(2000);

  Serial.println("[robot-eyes] Eyes initialized and ready");
}

/**
 * Main loop: Animate and render eyes.
 */
void loop() {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
  lastUpdateTime = currentTime;

  // Blinking logic
  if (currentTime - anim.lastBlinkTime > anim.blinkInterval && anim.blinkState == 0) {
    anim.blinkState = 1;
    anim.lastBlinkTime = currentTime;
  } else if (currentTime - anim.lastBlinkTime > anim.blinkDuration && anim.blinkState == 1) {
    anim.blinkState = 0;
    anim.lastBlinkTime = currentTime;
  }

  // Random eye movement
  if (currentTime - anim.lastMoveTime > random(1500, 3000) && anim.blinkState == 0) {
    int moveType = random(0, 8);
    switch (moveType) {
      case 0: leftEye.targetOffsetX = -8; leftEye.targetOffsetY = 0; break;  // left
      case 1: leftEye.targetOffsetX = 8; leftEye.targetOffsetY = 0; break;   // right
      case 2: leftEye.targetOffsetX = -8; leftEye.targetOffsetY = -6; break; // up-left
      case 3: leftEye.targetOffsetX = 8; leftEye.targetOffsetY = -6; break;  // up-right
      case 4: leftEye.targetOffsetX = -8; leftEye.targetOffsetY = 6; break;  // down-left
      case 5: leftEye.targetOffsetX = 8; leftEye.targetOffsetY = 6; break;   // down-right
      default: leftEye.targetOffsetX = 0; leftEye.targetOffsetY = 0; break;  // center
    }
    rightEye.targetOffsetX = leftEye.targetOffsetX;
    rightEye.targetOffsetY = leftEye.targetOffsetY;
    anim.lastMoveTime = currentTime;
  }

  // Update eye positions
  updateEyePosition(leftEye);
  updateEyePosition(rightEye);

  // Clear and render
  display.clearBuffer();

  // Draw eyes (skip during blink)
  if (anim.blinkState == 0) {
    drawEye(leftEye);
    drawEye(rightEye);
  } else {
    // Blink: draw horizontal lines
    display.drawLine(leftEye.baseX, leftEye.baseY + leftEye.height / 2,
                     leftEye.baseX + leftEye.width, leftEye.baseY + leftEye.height / 2);
    display.drawLine(rightEye.baseX, rightEye.baseY + rightEye.height / 2,
                     rightEye.baseX + rightEye.width, rightEye.baseY + rightEye.height / 2);
  }

  display.sendBuffer();
  delay(16);  // ~60 FPS
}
