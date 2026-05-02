/**
 * Robot Eyes on ESP32 with 128x64 OLED display.
 * Uses FluxGarage RoboEyes for animated eye rendering with auto-blink,
 * auto-move (idle mode), and joystick-controlled gaze direction.
 * A capacitive touch sensor on GPIO4 detects "petting": 3+ taps within
 * 2 seconds, or a continuous hold > 500 ms, triggers a happy laugh reaction.
 *
 * Wiring:
 *   OLED SDA -> GPIO21, SCL -> GPIO22 (hardware I2C)
 *   Joystick HORZ -> GPIO34, VERT -> GPIO35 (ADC-only, input-only pins)
 *   Touch sensor OUT -> GPIO4 (capacitive touch pin T0)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

// ---------- Hardware constants ----------

static constexpr uint8_t  DISPLAY_WIDTH     = 128;
static constexpr uint8_t  DISPLAY_HEIGHT    = 64;
static constexpr uint8_t  DISPLAY_I2C_ADDR  = 0x3C;
static constexpr int8_t   OLED_RESET_PIN    = -1;   // no dedicated reset line

static constexpr uint8_t  PIN_JOYSTICK_HORZ = 34;
static constexpr uint8_t  PIN_JOYSTICK_VERT = 35;

// GPIO4 maps to capacitive touch channel T0 on the ESP32.
// touchRead() returns a value inversely proportional to capacitance:
// low value = finger detected, high value = not touched.
static constexpr uint8_t  PIN_TOUCH         = 4;

// Threshold below which touchRead() counts as a touch event.
// Empirical value for Wokwi; adjust downward for noisier real hardware.
static constexpr uint32_t TOUCH_THRESHOLD   = 40;

// "Stroke" detection: 3 taps within this window counts as petting
static constexpr unsigned long TOUCH_TAP_WINDOW_MS   = 2000UL;
static constexpr uint8_t       TOUCH_TAP_MIN_COUNT   = 3;

// ...or a continuous hold longer than this is also a stroke
static constexpr unsigned long TOUCH_HOLD_MS         = 500UL;

// How long the HAPPY/laugh reaction is shown before reverting to DEFAULT
static constexpr unsigned long TOUCH_REACTION_MS     = 3000UL;

// ADC center value and dead-zone half-width (12-bit ADC, center ~2048)
static constexpr int      ADC_CENTER        = 2048;
static constexpr int      ADC_DEADZONE      = 300;

// Target frame rate passed to RoboEyes (library enforces it internally)
static constexpr uint8_t  TARGET_FPS        = 30;

// Frame period in milliseconds used for deltaTime budget calculation
static constexpr unsigned long FRAME_PERIOD_MS = 1000UL / TARGET_FPS;

// ---------- Globals ----------

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET_PIN);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// Touch state — all timing in milliseconds via millis()
static bool          touchWasActive      = false;  // touch state in the previous frame
static unsigned long touchPressStartMs   = 0;      // when the current press began
static uint8_t       tapCount            = 0;       // taps seen within the current window
static unsigned long firstTapMs          = 0;       // timestamp of the first tap in the window
static bool          reactionActive      = false;   // HAPPY reaction is currently playing
static unsigned long reactionStartMs     = 0;       // when the reaction began

// ---------- Helper ----------

/**
 * Read the capacitive touch pin and update stroke-detection state.
 * Triggers a HAPPY reaction when the robot is "petted":
 *   - 3 or more distinct taps within TOUCH_TAP_WINDOW_MS, or
 *   - a continuous touch held for at least TOUCH_HOLD_MS.
 * Non-blocking: uses millis() exclusively, no delay().
 *
 * @param nowMs  Current millis() timestamp passed in from loop()
 */
static void processTouchInput(unsigned long nowMs) {
    bool touchIsActive = (touchRead(PIN_TOUCH) < TOUCH_THRESHOLD);

    // ---- Detect rising edge (finger just placed) ----
    if (touchIsActive && !touchWasActive) {
        touchPressStartMs = nowMs;

        // Start or continue the tap-counting window
        if (tapCount == 0) {
            firstTapMs = nowMs;
        }
        tapCount++;
    }

    // ---- While finger is held: check for long-press stroke ----
    if (touchIsActive && !reactionActive) {
        unsigned long holdDuration = nowMs - touchPressStartMs;
        if (holdDuration >= TOUCH_HOLD_MS) {
            reactionActive    = true;
            reactionStartMs   = nowMs;
            tapCount          = 0;  // reset so the hold doesn't double-count
            roboEyes.setMood(HAPPY);
            roboEyes.anim_laugh();
            Serial.println("[touch] Stroke detected (long hold) — HAPPY");
        }
    }

    // ---- Detect falling edge (finger lifted): evaluate tap-count window ----
    if (!touchIsActive && touchWasActive) {
        if (!reactionActive && tapCount >= TOUCH_TAP_MIN_COUNT) {
            unsigned long windowDuration = nowMs - firstTapMs;
            if (windowDuration <= TOUCH_TAP_WINDOW_MS) {
                reactionActive  = true;
                reactionStartMs = nowMs;
                roboEyes.setMood(HAPPY);
                roboEyes.anim_laugh();
                Serial.println("[touch] Stroke detected (tap sequence) — HAPPY");
            }
        }
    }

    // ---- Expire the tap window if no tap arrived in time ----
    if (tapCount > 0 && !touchIsActive) {
        unsigned long windowAge = nowMs - firstTapMs;
        if (windowAge > TOUCH_TAP_WINDOW_MS) {
            tapCount = 0;
        }
    }

    // ---- End reaction after TOUCH_REACTION_MS and revert to DEFAULT ----
    if (reactionActive && (nowMs - reactionStartMs >= TOUCH_REACTION_MS)) {
        reactionActive = false;
        roboEyes.setMood(DEFAULT);
        Serial.println("[touch] Reaction ended — back to DEFAULT");
    }

    touchWasActive = touchIsActive;
}

/**
 * Map joystick ADC readings to one of the eight RoboEyes cardinal positions.
 * The center dead-zone returns DEFAULT so eyes rest in the middle.
 *
 * @param horzRaw  Raw ADC value for horizontal axis (0-4095)
 * @param vertRaw  Raw ADC value for vertical axis   (0-4095)
 * @return         RoboEyes position constant
 */
static uint8_t joystickToPosition(int horzRaw, int vertRaw) {
    // Translate raw ADC to signed offset from center
    int horzOffset = horzRaw - ADC_CENTER;
    int vertOffset = vertRaw - ADC_CENTER;  // positive = joystick pushed forward (up on display)

    bool horzLeft  = horzOffset < -ADC_DEADZONE;
    bool horzRight = horzOffset >  ADC_DEADZONE;
    // Joystick VERT axis: pushing forward decreases ADC value on analog joystick
    bool vertUp    = vertOffset < -ADC_DEADZONE;
    bool vertDown  = vertOffset >  ADC_DEADZONE;

    if (vertUp   && horzLeft)  { return NW; }
    if (vertUp   && horzRight) { return NE; }
    if (vertDown && horzLeft)  { return SW; }
    if (vertDown && horzRight) { return SE; }
    if (vertUp)                { return N;  }
    if (vertDown)              { return S;  }
    if (horzLeft)              { return W;  }
    if (horzRight)             { return E;  }

    return DEFAULT;
}

// ---------- Arduino lifecycle ----------

void setup() {
    Serial.begin(115200);
    Serial.println("[robot-eyes] Booting...");

    // GPIO34/35 are input-only on ESP32 — no pinMode needed, but explicit is safer
    pinMode(PIN_JOYSTICK_HORZ, INPUT);
    pinMode(PIN_JOYSTICK_VERT, INPUT);

    // Touch pins are capacitive and need no pinMode; touchRead() works directly.
    // The initial read is discarded so the first real read starts from a clean baseline.
    (void)touchRead(PIN_TOUCH);

    // Initialise I2C on the hardware pins wired in diagram.json
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDR)) {
        Serial.println("[robot-eyes] ERROR: SSD1306 not found — halting");
        // Spin forever; Serial output helps Wokwi / hardware debugging
        while (true) { delay(1000); }
    }

    display.clearDisplay();
    display.display();

    // Display object is passed via constructor — begin() only needs dimensions and FPS
    roboEyes.begin(DISPLAY_WIDTH, DISPLAY_HEIGHT, TARGET_FPS);

    // Monochrome: background = 0 (black), foreground = 1 (white)
    roboEyes.setDisplayColors(0, 1);

    // Eye geometry — sized to fill most of the 128x64 screen comfortably
    roboEyes.setWidth(38, 38);
    roboEyes.setHeight(28, 28);
    roboEyes.setBorderradius(6, 6);
    roboEyes.setSpacebetween(16);

    // Autonomous blink every ~3 s with ±2 s variation
    roboEyes.setAutoblinker(ON, 3, 2);

    // Idle drift: random gaze change every ~4 s with ±2 s variation.
    // This is overridden each frame when the joystick is outside the dead-zone,
    // so idle mode acts as the fallback when the joystick rests at center.
    roboEyes.setIdleMode(ON, 4, 2);

    roboEyes.setMood(DEFAULT);

    Serial.println("[robot-eyes] Ready");
}

void loop() {
    unsigned long frameStart = millis();

    // Process touch input first so a stroke reaction can override the mood
    // before roboEyes.update() renders the frame.
    processTouchInput(frameStart);

    // Read joystick and translate to a gaze position
    int horzRaw = analogRead(PIN_JOYSTICK_HORZ);
    int vertRaw = analogRead(PIN_JOYSTICK_VERT);
    uint8_t gazePosition = joystickToPosition(horzRaw, vertRaw);

    // Directly set eye position when the joystick is deflected.
    // When it returns to center (DEFAULT), idle mode takes over again.
    roboEyes.setPosition(gazePosition);

    // update() handles framerate throttling, auto-blink, idle movement,
    // clearing the buffer, drawing eyes and flushing to the display.
    roboEyes.update();

    // Spend any remaining frame budget in a tight loop rather than delay(),
    // so we stay responsive to future serial commands or interrupts.
    unsigned long elapsed = millis() - frameStart;
    if (elapsed < FRAME_PERIOD_MS) {
        delay(FRAME_PERIOD_MS - elapsed);
    }
}
