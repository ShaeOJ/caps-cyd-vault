# 🛡️ CAPS CYD Vault (Wasteland Cold Hardware Edition)

> **100% Air-Gapped Cold Hardware Wallet for Bottlecaps (BCAP)** built for the **ESP32-2432S028R ("Cheap Yellow Display" 2.8")** with MicroSD PSBT Signing and On-Screen High-Contrast QR Code Air-Gap.

---

## 🌟 Key Features

* **True Air-Gap Security:**
  * WiFi and Bluetooth radios are completely uncompiled and disabled in hardware. Zero network attack surface.
  * No USB data cables needed during signing. Operates purely on power (power bank or wall adapter).
* **Coldcard-Style MicroSD PSBT Signing:**
  * Web Wallet or desktop outputs an unsigned `.psbt` transaction file to a MicroSD card.
  * Insert MicroSD into the CYD 2.8" slot.
  * The CYD parses inputs, destination address, and satoshi amounts directly from the hardware screen.
  * Cryptographically signs with native `secp256k1` ECDSA upon user touchscreen approval.
  * Saves `/signed_tx.psbt` back to the card.
* **Hybrid Screen QR Display:**
  * **Receive Mode:** Displays your native SegWit (`btlcap1q...`) address as a sharp, high-contrast QR code directly on the 2.8" color display.
  * **Broadcast Mode:** After signing, displays the signed transaction as an on-screen QR code so a smartphone or webcam can scan and broadcast it instantly without even removing the SD card!
* **Memory Zeroization & Ephemeral Key Safety:**
  * Seed phrases and derived private keys live exclusively in volatile SRAM.
  * Dedicated **"Lock & Wipe"** mode actively scrubs all key buffers from memory using `memzero`.
  * Powering off the device instantly destroys all cryptographic keys.
* **Pip-Boy / Wasteland Aesthetics:**
  * Retro phosphor green (`#00FF00`) and wasteland amber CRT interface designed for the 320x240 ILI9341 display.
  * Dynamic status indicator via the onboard RGB LED (Green = Active, Blue = Reviewing/Signing, Red = Locked).

---

## 📐 Hardware Specifications: ESP32-2432S028R (CYD 2.8")

| Component | Specification |
| :--- | :--- |
| **Microcontroller** | ESP32-D0WDQ6-V3 Dual-Core Xtensa @ 240 MHz |
| **Memory** | 520 KB SRAM, 4 MB SPI Flash |
| **Display** | 2.8-inch Color TFT LCD (320x240 pixels), ILI9341 Driver |
| **Touchscreen** | XPT2046 4-Wire Resistive Touch |
| **Storage** | MicroSD Card Slot (SPI Mode) |
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

### Requirements
1. [PlatformIO Core](https://platformio.org/) installed (`pio --version`).
2. USB-to-UART Driver (CH340 or CP2102 depending on CYD revision).

### Commands
```bash
# Clone the repository
git clone https://github.com/ShaeOJ/caps-cyd-vault.git
cd caps-cyd-vault

# Build the firmware
pio run -e cyd28r

# Flash to the CYD (Hold BOOT, tap RST to enter download mode, then flash):
pio run -e cyd28r -t upload --upload-port COM4

# Open Serial Monitor
pio device monitor -b 115200 -p COM4
```

---

## 📖 Step-by-Step User Guide

### 1. Initializing Your Seed Phrase
* **Default Mode:** Boots with a test mnemonic if no custom seed is present.
* **Custom Cold Storage:** Create a plain text file named `seed.txt` on the root of your MicroSD card containing your 12- or 24-word recovery phrase:
  ```text
  abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about
  ```
* Insert the MicroSD card into the CYD and boot. The vault will read the seed into volatile RAM and initialize your BIP84 (`m/84'/0'/0'/0/0`) address.
* *(Optional)* Once loaded into RAM, you can delete `seed.txt` from the card for stateless operation.

### 2. Receiving Bottlecaps (BCAP)
1. From the Dashboard, tap **`[ 1. RECEIVE & SHOW QR CODE ]`**.
2. A large, high-contrast QR code representing your native SegWit `btlcap1q...` address appears on screen.
3. Tap **`< PREV`** or **`NEXT >`** to cycle between derivation indices (`#0`, `#1`, `#2`, etc.).
4. Scan the QR code using any Bottlecaps wallet to deposit funds.

### 3. Signing Transactions (MicroSD Air-Gap)
1. In the **Bottlecaps Web Wallet** (`wallet.undergroundroots.ca`), prepare your transaction and click **Export PSBT** (saves as `tx.psbt`).
2. Copy `tx.psbt` to your MicroSD card.
3. Insert the card into the CYD 2.8 slot.
4. On the CYD Dashboard, tap **`[ 2. SIGN TRANSACTION (SD CARD) ]`**.
5. The air-gapped screen will display:
   * **RECIPIENT:** `btlcap1q9x...`
   * **AMOUNT:** `150.0000 BCAP`
   * **NETWORK FEE:** `0.0001 BCAP`
   * **INPUTS / OUTPUTS:** count
6. Verify the details match what you intended.
7. Tap **`[ V APPROVE & SIGN ]`**.
8. The ESP32 signs all inputs with `secp256k1` in ~20 milliseconds, writes `/signed_tx.psbt` to the SD card, and displays the **Signed Transaction QR code** on screen.

### 4. Broadcasting
* **Option A (Instant Phone/Webcam Scan):** Open your online wallet and scan the signed QR code directly off the CYD display to broadcast!
* **Option B (SD Return):** Put the MicroSD card back into your PC and broadcast `/signed_tx.psbt`.

---

## 🔒 Security Guarantees

1. **Zero Radio Transmission:** ESP32 WiFi and Bluetooth basebands are uninitialized. The device cannot emit RF or connect to networks.
2. **Deterministic Derivation:** Uses BIP-84 native SegWit with coin type `0` (`m/84'/0'/0'/0/i`) fully conforming with the Bottlecaps core consensus and web wallet.
3. **RAM-Only Life Cycle:** Flash storage is never written with sensitive keys. All private keys are zeroized upon locking or loss of power.

---

## 📜 License
MIT License. Built for the Bottlecaps Wasteland Community.
