/**
 * Robot Eyes on ESP32 with 128x64 OLED display.
 * Uses FluxGarage RoboEyes for animated eye rendering with auto-blink,
 * auto-move (idle mode), and joystick-controlled gaze direction.
 *
 * Wiring:
 *   OLED SDA -> GPIO21, SCL -> GPIO22 (hardware I2C)
 *   Joystick HORZ -> GPIO34, VERT -> GPIO35 (ADC-only, input-only pins)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RoboEyes.h>

// ---------- Hardware constants ----------

static constexpr uint8_t  DISPLAY_WIDTH     = 128;
static constexpr uint8_t  DISPLAY_HEIGHT    = 64;
static constexpr uint8_t  DISPLAY_I2C_ADDR  = 0x3C;
static constexpr int8_t   OLED_RESET_PIN    = -1;   // no dedicated reset line

static constexpr uint8_t  PIN_JOYSTICK_HORZ = 34;
static constexpr uint8_t  PIN_JOYSTICK_VERT = 35;

// ADC center value and dead-zone half-width (12-bit ADC, center ~2048)
static constexpr int      ADC_CENTER        = 2048;
static constexpr int      ADC_DEADZONE      = 300;

// Target frame rate passed to RoboEyes (library enforces it internally)
static constexpr uint8_t  TARGET_FPS        = 30;

// Frame period in milliseconds used for deltaTime budget calculation
static constexpr unsigned long FRAME_PERIOD_MS = 1000UL / TARGET_FPS;

// ---------- Globals ----------

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET_PIN);
RoboEyes roboEyes;

// ---------- Helper ----------

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

    // Initialise I2C on the hardware pins wired in diagram.json
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDR)) {
        Serial.println("[robot-eyes] ERROR: SSD1306 not found — halting");
        // Spin forever; Serial output helps Wokwi / hardware debugging
        while (true) { delay(1000); }
    }

    display.clearDisplay();
    display.display();

    // Hand the Adafruit display object to RoboEyes
    roboEyes.begin(DISPLAY_WIDTH, DISPLAY_HEIGHT, TARGET_FPS, display);

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

    // Read joystick and translate to a gaze position
    int horzRaw = analogRead(PIN_JOYSTICK_HORZ);
    int vertRaw = analogRead(PIN_JOYSTICK_VERT);
    uint8_t gazePosition = joystickToPosition(horzRaw, vertRaw);

    // Directly set eye position when the joystick is deflected.
    // When it returns to center (DEFAULT), idle mode takes over again.
    roboEyes.setPosition(gazePosition);

    // RoboEyes::update() handles framerate throttling, auto-blink, and
    // idle movement internally — no blocking delay required here.
    roboEyes.update();

    display.clearDisplay();
    roboEyes.drawEyes();
    display.display();

    // Spend any remaining frame budget in a tight loop rather than delay(),
    // so we stay responsive to future serial commands or interrupts.
    unsigned long elapsed = millis() - frameStart;
    if (elapsed < FRAME_PERIOD_MS) {
        delay(FRAME_PERIOD_MS - elapsed);
    }
}
