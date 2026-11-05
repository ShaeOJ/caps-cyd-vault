#include "vault_ui.h"
#include <SPI.h>

VaultUI UI;

VaultUI::VaultUI() : ts(TOUCH_CS, TOUCH_IRQ) {
}

void VaultUI::begin() {
    // 1. Initialize RGB LED pins
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    setLed(false, false, false); // All off

    // 2. Initialize Display
    tft.init();
    tft.setRotation(1); // 320x240 landscape
    tft.fillScreen(COLOR_BG);

    // Turn on backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // 3. Initialize Touchscreen SPI
    SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin();
    ts.setRotation(1);

    Serial.println("[UI] Display and Touch initialized (320x240)");
}

void VaultUI::setLed(bool r, bool g, bool b) {
    // Active LOW on CYD
    digitalWrite(LED_R, r ? LOW : HIGH);
    digitalWrite(LED_G, g ? LOW : HIGH);
    digitalWrite(LED_B, b ? LOW : HIGH);
}

bool VaultUI::getTouch(int16_t& x, int16_t& y) {
    if (!ts.touched()) return false;
    TS_Point p = ts.getPoint();

    // Debounce / pressure threshold
    if (p.z < 200) return false;

    // CYD 2.8" Resistive Touch calibration mapping
    // Landscape rotation: raw X ~200-3800, raw Y ~250-3850
    int16_t mappedX = map(p.x, 200, 3800, 0, SCREEN_WIDTH);
    int16_t mappedY = map(p.y, 250, 3850, 0, SCREEN_HEIGHT);

    // Clamp bounds
    if (mappedX < 0) mappedX = 0;
    if (mappedX >= SCREEN_WIDTH) mappedX = SCREEN_WIDTH - 1;
    if (mappedY < 0) mappedY = 0;
    if (mappedY >= SCREEN_HEIGHT) mappedY = SCREEN_HEIGHT - 1;

    x = mappedX;
    y = mappedY;
    return true;
}

void VaultUI::drawHeader(const char* title, bool sdOk, bool unlocked) {
    tft.fillRect(0, 0, SCREEN_WIDTH, 22, COLOR_DARK_GRAY);
    tft.drawFastHLine(0, 22, SCREEN_WIDTH, COLOR_PHOSPHOR);

    tft.setTextColor(COLOR_PHOSPHOR, COLOR_DARK_GRAY);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(title, 6, 4, 2);

    // SD Badge
    tft.setTextDatum(TR_DATUM);
    if (sdOk) {
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_DARK_GRAY);
        tft.drawString("[SD:OK]", SCREEN_WIDTH - 70, 4, 2);
    } else {
        tft.setTextColor(COLOR_ALERT_RED, COLOR_DARK_GRAY);
        tft.drawString("[NO-SD]", SCREEN_WIDTH - 70, 4, 2);
    }

    // Vault Status Badge
    if (unlocked) {
        tft.setTextColor(COLOR_AMBER, COLOR_DARK_GRAY);
        tft.drawString("[UNLOCKED]", SCREEN_WIDTH - 6, 4, 2);
    } else {
        tft.setTextColor(COLOR_ALERT_RED, COLOR_DARK_GRAY);
        tft.drawString("[LOCKED]", SCREEN_WIDTH - 6, 4, 2);
    }
}

void VaultUI::drawButton(int x, int y, int w, int h, const char* label, uint16_t borderColor, uint16_t textColor) {
    tft.drawRect(x, y, w, h, borderColor);
    tft.drawRect(x + 1, y + 1, w - 2, h - 2, borderColor);
    tft.setTextColor(textColor, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(label, x + (w / 2), y + (h / 2), 2);
}

void VaultUI::showSplash() {
    setLed(false, true, false); // Green
    tft.fillScreen(COLOR_BG);

    // Border Frame
    tft.drawRect(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, COLOR_PHOSPHOR);
    tft.drawRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, COLOR_PHOSPHOR_DIM);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("=== WASTELAND COLD VAULT ===", SCREEN_WIDTH / 2, 16, 2);

    tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
    tft.drawString("CYD-2.8 HARDWARE EDITION", SCREEN_WIDTH / 2, 38, 2);

    tft.setTextDatum(TL_DATUM);
    int y = 68;
    tft.drawString("> ESP32-D0WDQ6 @ 240MHz: READY", 16, y, 2); y += 20;
    tft.drawString("> CRYPTO ENGINE (secp256k1): READY", 16, y, 2); y += 20;
    tft.drawString("> AIR-GAP: WiFi/BT Radios OFF", 16, y, 2); y += 20;
    tft.drawString("> NETWORK: Bottlecaps (btlcap1)", 16, y, 2); y += 20;
    tft.drawString("> STORAGE: MicroSD PSBT Engine", 16, y, 2); y += 26;

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("[ TAP SCREEN TO ENTER VAULT ]", SCREEN_WIDTH / 2, y, 2);
}

void VaultUI::showDashboard(const VaultState& state) {
    setLed(false, state.isUnlocked, !state.isUnlocked); // Green if unlocked, Blue if locked
    tft.fillScreen(COLOR_BG);
    drawHeader("CAPS CYD VAULT", state.sdMounted, state.isUnlocked);

    // Active Address Card Box
    tft.drawRect(8, 30, SCREEN_WIDTH - 16, 56, COLOR_PHOSPHOR_DIM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    char buf[64];
    snprintf(buf, sizeof(buf), "ACTIVE RECEIVE ADDRESS (#%u):", state.addressIndex);
    tft.drawString(buf, 14, 34, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    if (strlen(state.currentAddress) > 0) {
        tft.drawString(state.currentAddress, 14, 52, 2);
    } else {
        tft.drawString("No address loaded (Locked)", 14, 52, 2);
    }

    // Touch Action Buttons
    // Button 1: Show Receive QR
    drawButton(8, 96, SCREEN_WIDTH - 16, 38, "1. RECEIVE & SHOW QR CODE", COLOR_PHOSPHOR, COLOR_PHOSPHOR);

    // Button 2: Sign Transaction from SD Card
    drawButton(8, 142, SCREEN_WIDTH - 16, 38, "2. SIGN TRANSACTION (SD CARD)", COLOR_AMBER, COLOR_AMBER);

    // Button 3: Lock Vault
    drawButton(8, 188, SCREEN_WIDTH - 16, 38, "3. LOCK & WIPE KEYS", COLOR_ALERT_RED, COLOR_ALERT_RED);
}

void VaultUI::renderQR(const char* payload, int x, int y, int scale) {
    size_t len = strlen(payload);
    // Determine required QR version based on length (Binary mode)
    // Version 3: up to 55 bytes; Version 5: up to 106 bytes; Version 8: up to 192 bytes
    uint8_t version = 3;
    if (len > 120) version = 8;
    else if (len > 80) version = 6;
    else if (len > 50) version = 4;

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(8)]; // allocate for up to ver 8
    qrcode_initText(&qrcode, qrcodeData, version, 0, payload);

    int size = qrcode.size;
    int totalPx = size * scale;

    // White quiet-zone border
    tft.fillRect(x - 4, y - 4, totalPx + 8, totalPx + 8, COLOR_WHITE);

    for (uint8_t qy = 0; qy < size; qy++) {
        for (uint8_t qx = 0; qx < size; qx++) {
            uint16_t color = qrcode_getModule(&qrcode, qx, qy) ? COLOR_BG : COLOR_WHITE;
            tft.fillRect(x + (qx * scale), y + (qy * scale), scale, scale, color);
        }
    }
}

void VaultUI::showReceiveQR(const char* address, uint32_t index) {
    tft.fillScreen(COLOR_BG);
    drawHeader("RECEIVE BOTTLECAPS", true, true);

    // Render centered QR Code (scale 3)
    // At ver 4 (33x33) -> 99px wide
    int qrX = (SCREEN_WIDTH / 2) - 52;
    int qrY = 30;
    renderQR(address, qrX, qrY, 3);

    // Show Address text beneath QR code
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    char idxBuf[32];
    snprintf(idxBuf, sizeof(idxBuf), "Derivation Index: m/84'/0'/0'/0/%u", index);
    tft.drawString(idxBuf, SCREEN_WIDTH / 2, 142, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString(address, SCREEN_WIDTH / 2, 162, 2);

    // Navigation Buttons
    drawButton(8, 196, 96, 36, "< PREV", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
    drawButton(112, 196, 96, 36, "BACK", COLOR_AMBER, COLOR_AMBER);
    drawButton(216, 196, 96, 36, "NEXT >", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showSignReview(const UnsignedTxDetails& tx) {
    setLed(false, false, true); // Blue (Signing inspection)
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSACTION REVIEW", true, true);

    tft.drawRect(6, 28, SCREEN_WIDTH - 12, 154, COLOR_AMBER);
    tft.setTextDatum(TL_DATUM);

    // Destination
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("PAY TO:", 12, 34, 2);
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString(tx.recipient, 12, 52, 2);

    // Amount
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("AMOUNT:", 12, 74, 2);
    tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
    double capsAmt = (double)tx.amountSats / 1e8;
    char amtBuf[32];
    snprintf(amtBuf, sizeof(amtBuf), "%.4f BCAP", capsAmt);
    tft.drawString(amtBuf, 86, 74, 4);

    // Fee & Inputs
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("FEE:", 12, 108, 2);
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    double capsFee = (double)tx.feeSats / 1e8;
    char feeBuf[32];
    snprintf(feeBuf, sizeof(feeBuf), "%.6f BCAP", capsFee);
    tft.drawString(feeBuf, 50, 108, 2);

    char metaBuf[48];
    snprintf(metaBuf, sizeof(metaBuf), "Inputs: %u  |  Outputs: %u", tx.inputCount, tx.outputCount);
    tft.drawString(metaBuf, 12, 130, 2);

    tft.setTextColor(COLOR_ALERT_RED, COLOR_BG);
    tft.drawString("! VERIFY RECIPIENT ON AIR-GAPPED SCREEN !", 12, 152, 2);

    // Bottom Choice Buttons
    drawButton(8, 190, 148, 42, "X REJECT", COLOR_ALERT_RED, COLOR_ALERT_RED);
    drawButton(164, 190, 148, 42, "V APPROVE & SIGN", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showSignedSuccess(const char* signedBase64, const char* filename) {
    setLed(false, true, false); // Green
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSACTION SIGNED!", true, true);

    // Render Signed Tx QR Code
    // Version 8 at scale 2 -> ~100px
    int qrX = (SCREEN_WIDTH / 2) - 52;
    int qrY = 30;
    renderQR(signedBase64, qrX, qrY, 2);

    tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SAVED TO SD CARD:", SCREEN_WIDTH / 2, 146, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    char buf[64];
    snprintf(buf, sizeof(buf), "signed_%s", filename);
    tft.drawString(buf, SCREEN_WIDTH / 2, 166, 2);

    drawButton(60, 194, 200, 38, "RETURN TO DASHBOARD", COLOR_AMBER, COLOR_AMBER);
}

void VaultUI::showLocked() {
    setLed(true, false, false); // Red
    tft.fillScreen(COLOR_BG);
    drawHeader("VAULT LOCKED", false, false);

    tft.drawRect(8, 36, SCREEN_WIDTH - 16, 140, COLOR_ALERT_RED);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_ALERT_RED, COLOR_BG);
    tft.drawString("MEMORY ZEROIZED & LOCKED", SCREEN_WIDTH / 2, 54, 4);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString("All private keys and seeds have been", SCREEN_WIDTH / 2, 94, 2);
    tft.drawString("scrubbed from RAM buffers.", SCREEN_WIDTH / 2, 114, 2);
    tft.drawString("No keys exist on flash storage.", SCREEN_WIDTH / 2, 134, 2);

    drawButton(40, 190, 240, 40, "INITIALIZE / UNLOCK", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showMessage(const char* title, const char* msg, uint16_t color) {
    tft.fillRect(20, 60, SCREEN_WIDTH - 40, 120, COLOR_BG);
    tft.drawRect(20, 60, SCREEN_WIDTH - 40, 120, color);
    tft.drawRect(22, 62, SCREEN_WIDTH - 44, 116, color);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(color, COLOR_BG);
    tft.drawString(title, SCREEN_WIDTH / 2, 74, 4);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString(msg, SCREEN_WIDTH / 2, 118, 2);
}
