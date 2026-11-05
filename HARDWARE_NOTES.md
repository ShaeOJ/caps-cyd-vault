# CAPS CYD Vault — Hardware Diagnosis & Status Handover

> **Hardware Device Confirmed:** **ESP32-3248S035** (3.5" 480x320 ST7796 Display + GT911 Capacitive Touch)
> **Active COM Port:** `COM4` (CH340 USB-UART)
> **Target Repository:** `F:\GitHub\caps-cyd-vault`
> **Reference Working Project:** `F:\GitHub\CYD-External-Display-for-Bitaxe`

---

## 1. Hardware Identity Discovered

The board is an **ESP32-3248S035** (3.5" display), not the 2.8" CYD:
- **Display Controller:** `ST7796` (480x320 landscape)
- **Backlight Pin:** **GPIO 27** (Active HIGH)
- **Touch Controller:** **GT911 Capacitive Touch** (I2C)
  - `TOUCH_SDA = 33`
  - `TOUCH_SCL = 32`
  - `TOUCH_INT = 21`
  - `TOUCH_RST = 25`
  - Native Resolution: `480 x 320`
  - Rotation: `ROTATION_LEFT`
- **MicroSD Card Bus:** VSPI (`CS=5, SCK=18, MISO=19, MOSI=23`) — **Tested and working (7,861 MB mounted)**.
- **Physical BOOT Button:** `GPIO 0` — **Tested and working as hardware click companion**.

---

## 2. Root Cause of Previous Touch & Display Issues

1. **Backlight Clashing with Touch Interrupt (GPIO 21):**
   - In standard 2.8" CYD definitions, backlight is `GPIO 21`.
   - In `platformio.ini`, `TFT_BL=21` was active. When `tft.init()` executed, the `TFT_eSPI` driver configured GPIO 21 as `OUTPUT HIGH`.
   - On the 3.5" board, **GPIO 21 is `TOUCH_INT` for the GT911**. Clamping it HIGH prevented the GT911 from completing its hardware address latching and caused I2C NACKs (`Error 263`).
   - **Fix:** Switched `TFT_BL` to **GPIO 27** in `platformio.ini`.

2. **Bitbang SPI Overriding I2C Bus:**
   - Previous fallback touch code was bitbanging pins 33 (CS), 32 (MOSI), and 25 (CLK) as pseudo-SPI for an XPT2046.
   - Every millisecond when no touch was active, it pulled pin 25 LOW (GT911 hardware RESET!) and pin 32 LOW (GT911 I2C SCL).
   - **Fix:** Completely eliminated the legacy XPT2046 bitbang code and CST816 fallbacks. The GT911 is now the sole clean I2C touch engine.

3. **I2C Read Stalling (Error 263 Timeout):**
   - The ESP32 Wire library defaults to a 1000ms timeout per failed I2C transaction.
   - When checking an unacknowledged address (0x5D vs 0x14), the CPU stalled for 1 second each cycle.
   - **Fix:** Added `Wire.setTimeOut(20)` so any failed touch read returns in 20 milliseconds without perceptible lag.

---

## 3. Current Firmware State

The codebase in `F:\GitHub\caps-cyd-vault` is fully compiled and ready:
- `platformio.ini`:
  - `ST7796_DRIVER=1`
  - `TFT_WIDTH=320`, `TFT_HEIGHT=480` (Landscape: 480x320)
  - `TFT_BL=27`, `TFT_BACKLIGHT_ON=HIGH`
  - `TFT_RGB_ORDER=TFT_BGR`
  - `SPI_FREQUENCY=65000000`
- `include/config.h`:
  - `SCREEN_WIDTH = 480`
  - `SCREEN_HEIGHT = 320`
- `src/ui/vault_ui.cpp`:
  - All screens (`showSplash`, `showDashboard`, `showReceiveQR`, `showCardGallery`, `showSignReview`, `showSignedSuccess`, `showLocked`) upgraded to 480x320 layout with Pip-Boy green & amber styling.
  - Automatic I2C bus scanner on boot (`0x5D` vs `0x14`).
  - GT911 landscape coordinate inversion: `x = 480 - ts.points[0].x`, `y = 320 - ts.points[0].y`.
  - BOOT button on GPIO 0 retained as instant hardware click.
- `src/main.cpp`:
  - All touch bounding boxes aligned with 480x320 layout.

---

## 4. How to Resume When Ready

When you return:
1. Connect the CYD to USB (`COM4`).
2. Put the ESP32 in download mode:
   - **Hold down BOOT**, tap **RST** once, release **BOOT**.
3. In terminal, run:
   ```powershell
   pio run -d "F:\GitHub\caps-cyd-vault" -e cyd28r -t upload
   ```
4. Check serial output at 115200:
   - Verify `[I2C] Responding device found at 0x...`
   - Tap screen and observe `[TOUCH-GT911] Pressed at x=... y=...`
