# robot-eyes — Development Guide

## Overview
Robot Eyes on ESP32 with 64x128 OLED display. Simulated and tested in Wokwi.

## Setup

### PlatformIO
```bash
# Install dependencies
platformio lib install

# Build
platformio run --environment esp32dev

# Upload (if hardware connected)
platformio run --environment esp32dev --target upload
```

### Wokwi Simulation
```bash
wokwi-cli robot-eyes --host localhost --port 3000
```
Or use the Wokwi web editor by opening `wokwi.json` / `wokwi.toml`.

## Code Style
- **Naming:** camelCase (vars), UPPER_CASE (constants), descriptive names only
- **Braces:** always `{}`, even for single-line `if`/`for`
- **Comments:** explain WHY, not WHAT
- **Variable names:** no single-letter names (except `i`, `j` in tight loops)

## Git Workflow
- Branch types: `feature`, `bugfix`, `hotfix`
- Commit messages: German, CLAUDE.md format (see root)
- Always: build passes, code reviewed

## Files to Avoid
- Don't edit `build/` or `.pio/` (auto-generated)
- Don't commit binaries (`.bin`, `.elf`, `.hex`)

---
