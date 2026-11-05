#pragma once
#include <Arduino.h>

// ============================================================
// CAPS CYD Vault — Hardware Pin Configuration
// Board: ESP32-2432S028R (Cheap Yellow Display 2.8" ILI9341 + XPT2046)
// ============================================================

// Display (TFT_eSPI handles these via platformio.ini build_flags):
// MOSI=13, SCLK=14, CS=15, DC=2, RST=12, BL=21

// Touchscreen (XPT2046 resistive)
#ifndef TOUCH_CS
#define TOUCH_CS   33
#endif
#ifndef TOUCH_IRQ
#define TOUCH_IRQ  36
#endif
#ifndef TOUCH_CLK
#define TOUCH_CLK  25
#endif
#ifndef TOUCH_MOSI
#define TOUCH_MOSI 32
#endif
#ifndef TOUCH_MISO
#define TOUCH_MISO 39
#endif

// MicroSD Card (VSPI bus)
#ifndef SD_CS
#define SD_CS      5
#endif
#ifndef SD_SCK
#define SD_SCK     18
#endif
#ifndef SD_MISO
#define SD_MISO    19
#endif
#ifndef SD_MOSI
#define SD_MOSI    23
#endif

// Onboard RGB Status LED (Active LOW on CYD)
#ifndef LED_R
#define LED_R      4
#endif
#ifndef LED_G
#define LED_G      16
#endif
#ifndef LED_B
#define LED_B      17
#endif

// Screen Dimensions
#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240

// UI Theme: Pip-Boy / Wasteland Phosphor Palette (RGB565)
#define COLOR_BG          0x0000 // Pure Black
#define COLOR_PHOSPHOR    0x07E0 // Bright Pip-Boy Green
#define COLOR_PHOSPHOR_DIM 0x03E0 // Dim Green
#define COLOR_AMBER       0xFDA0 // Wasteland Amber
#define COLOR_AMBER_DIM   0x9B80 // Dim Amber
#define COLOR_ALERT_RED   0xF800 // Alert Red
#define COLOR_WHITE       0xFFFF // Pure White
#define COLOR_DARK_GRAY   0x2104 // Dark Panel Gray
#define COLOR_GRID_LINE   0x01E0 // Subtle Grid Green

// Bottlecaps Network Parameters
#define CAPS_HRP          "btlcap"
#define CAPS_REGTEST_HRP  "btlcaprt"
#define CAPS_PUBKEY_HASH  0x19 // 'B' (base58)
#define CAPS_SCRIPT_HASH  0x32 // 'M' (base58)
#define CAPS_WIF          0x99
#define CAPS_COINTYPE     0
#define CAPS_PURPOSE      84   // BIP84 Native SegWit (P2WPKH)

// Default Derivation Path
#define CAPS_DERIVATION_PATH "m/84'/0'/0'/0/0"
