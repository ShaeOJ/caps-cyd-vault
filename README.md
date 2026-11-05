# 🛡️ CAPS CYD Vault (Wasteland Cold Hardware Edition)

> **100% Air-Gapped Cold Hardware Wallet for Bottlecaps (CAPS) & Sidechain (BCAP)** built for the **ESP32-2432S028R ("Cheap Yellow Display" 2.8")** with MicroSD PSBT Signing, On-Screen QR Air-Gap, and Offline Wasteland Collectible Card Gallery.

---

## 🌟 Key Features

* **Dual-Chain Cold Storage (CAPS Mainnet & BCAP Sidechain):**
  * Switch seamlessly on device between **CAPS Mainnet** (`btlcap1q...` via `m/84'/0'/0'/0/i`) and **BCAP Sidechain** (`bcap1q...` via `m/84'/1'/0'/0/i`).
  * One single recovery seed secures your balances and cards across both chains.
* **Offline Wasteland Card Inventory & Pokédex:**
  * Displays your collectible bottlecap cards in full 16-bit color directly on the 2.8" TFT display!
  * Scans `/cards/` on the MicroSD card for `.bmp` artwork and `.json` metadata (Serial #, Tier 1–5, Title, Recipe Hash).
  * Smooth touch navigation (`< PREV`, `NEXT >`, `BACK`) to showcase your collection in true wasteland style.
* **"What You See Is What You Sign" (WYSIWYS) for Card Swaps:**
  * When trading or listing cards on the **Wasteland Trading Post**, the CYD pulls up the visual card artwork side-by-side with trade amounts and fees before signing. No blind signing!
* **Coldcard-Style MicroSD PSBT Signing:**
  * Web Wallet or desktop outputs an unsigned `.psbt` transaction file to a MicroSD card.
  * Insert MicroSD into the CYD 2.8" slot.
  * The CYD parses inputs, destination address, and satoshi amounts directly from the hardware screen.
  * Cryptographically signs with native `secp256k1` ECDSA upon user touchscreen approval.
  * Saves `/signed_tx.psbt` back to the card.
* **Hybrid Screen QR Display:**
  * **Receive Mode:** Displays your native SegWit (`btlcap1q...` or `bcap1q...`) address as a sharp, high-contrast QR code directly on the 2.8" color display.
  * **Broadcast Mode:** After signing, displays the signed transaction as an on-screen QR code so a smartphone or webcam can scan and broadcast it immediately without needing to remove the SD card!
* **Memory Zeroization & Ephemeral Key Safety:**
  * Seed phrases and derived private keys live exclusively in volatile SRAM.
  * Dedicated **"Lock & Wipe"** mode actively scrubs all key buffers from memory using `memzero`.
  * Powering off the device instantly destroys all cryptographic keys.

---

## 🎴 MicroSD Directory Structure for Cards

Place your card artwork and metadata in the `/cards/` folder on your MicroSD card:

```
MicroSD Card:
├── seed.txt                  (Optional seed recovery phrase)
├── /cards/
│   ├── card_0001.bmp         (16-bit or 24-bit BMP image, e.g. 120x150 or 140x160)
│   ├── card_0001.json        (Card metadata)
│   ├── card_0042.bmp
│   └── card_0042.json
└── tx.psbt                   (Unsigned transaction)
```

### Example `card_0042.json`
```json
{
  "serial": 42,
  "tier": 4,
  "title": "Quantum Bottlecap",
  "recipe_hash": "3a4b7c89e1d2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8"
}
```

Card tiers are dynamically color-coded with the Wasteland palette:
* **Tier 1 (Scrap):** Gray (`#8410`)
* **Tier 2 (Standard):** Pip-Boy Green (`#07E0`)
* **Tier 3 (Refined):** Cyan Blue (`#051F`)
* **Tier 4 (Quantum):** Violet Purple (`#A01F`)
* **Tier 5 (Forged):** Gold / Amber (`#FDE0`)

---

## 📐 Hardware Specifications: ESP32-2432S028R (CYD 2.8")

| Component | Specification |
| :--- | :--- |
| **Microcontroller** | ESP32-D0WDQ6-V3 Dual-Core Xtensa @ 240 MHz |
| **Memory** | 520 KB SRAM, 4 MB SPI Flash |
| **Display** | 2.8-inch Color TFT LCD (320x240 pixels), ILI9341 Driver |
| **Touchscreen** | XPT2046 4-Wire Resistive Touch |
| **Storage** | MicroSD Card Slot (VSPI Bus) |
| **Status LED** | Onboard Tricolor RGB LED (Active LOW) |
| **Power** | 5V via USB-C or Micro-USB |

---

## 🔌 Pinout Mapping

```
  ESP32-2432S028 Pinout:
  ├── ILI9341 Display (HSPI Bus):
  │   ├── MOSI: GPIO 13
  │   ├── SCLK: GPIO 14
  │   ├── CS:   GPIO 15
  │   ├── DC:   GPIO 2
  │   ├── RST:  GPIO 12
  │   └── BL:   GPIO 21 (Backlight)
  ├── XPT2046 Touch (Dedicated Soft SPI):
  │   ├── CLK:  GPIO 25
  │   ├── MISO: GPIO 39
  │   ├── MOSI: GPIO 32
  │   ├── CS:   GPIO 33
  │   └── IRQ:  GPIO 36
  ├── MicroSD Card (VSPI Bus):
  │   ├── CS:   GPIO 5
  │   ├── SCK:  GPIO 18
  │   ├── MISO: GPIO 19
  │   └── MOSI: GPIO 23
  └── Onboard RGB LED:
      ├── RED:   GPIO 4
      ├── GREEN: GPIO 16
      └── BLUE:  GPIO 17
```

---

## 🚀 Building & Flashing

```bash
# Clone the repository
git clone https://github.com/ShaeOJ/caps-cyd-vault.git
cd caps-cyd-vault

# Build the firmware
pio run -e cyd28r

# Flash to the CYD:
pio run -e cyd28r -t upload --upload-port COM4

# Open Serial Monitor
pio device monitor -b 115200 -p COM4
```

---

## 📜 License
MIT License. Built for the Bottlecaps Wasteland Community.
