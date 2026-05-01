# robot-eyes

Animated robot eyes on a 64x128 OLED display controlled by ESP32.

## Features
- Interactive eye animations
- I2C OLED display (SSD1306)
- Wokwi simulation support
- PlatformIO build system

## Hardware
- **Microcontroller:** ESP32 Dev Module
- **Display:** 64x128 OLED (SSD1306)
- **Communication:** I2C (SDA=GPIO21, SCL=GPIO22)

## Quick Start

### Build (PlatformIO)
```bash
platformio run --environment esp32dev
```

### Simulate (Wokwi)
```bash
wokwi-cli robot-eyes --host localhost --port 3000
```

### Upload to Hardware
```bash
platformio run --environment esp32dev --target upload
```

## Project Structure
```
robot-eyes/
├── src/
│   ├── main.cpp          Main application
│   └── display.cpp       Display controller
├── include/
│   └── display.h         Display interface
├── platformio.ini        Build configuration
├── wokwi.toml           Wokwi simulation config
├── brain.yaml           Project specification
└── CLAUDE.md            Development guide
```

## Development
See `CLAUDE.md` for code style and conventions.

See `brain.yaml` for project constraints and specifications.
