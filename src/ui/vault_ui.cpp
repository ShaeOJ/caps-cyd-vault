#include "vault_ui.h"
#include "../storage/sd_vault.h"

VaultUI UI;

VaultUI::VaultUI() : ts(33, 32, 21, 25, 480, 320) {
    gt911Ready = false;
}

void VaultUI::begin() {
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    // CYD RGB LEDs are active LOW — HIGH turns them off
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, HIGH);

    // Physical BOOT button on GPIO 0 as immediate companion / fallback input
    pinMode(0, INPUT_PULLUP);

    // Turn on backlight (GPIO 27 is the backlight on this board)
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    tft.init();
    tft.setRotation(1); // 480x320 landscape
    tft.fillScreen(COLOR_BG);

    // Ensure backlight pin stays HIGH
    digitalWrite(27, HIGH);

    // Initialize I2C with fast timeout so polling never stalls
    Wire.begin(33, 32);
    Wire.setTimeOut(20); // 20ms timeout instead of default 1000ms
    delay(50);

    // Scan I2C bus to discover touch address
    Serial.println("[I2C] Scanning I2C bus on SDA=33, SCL=32...");
    uint8_t detectedAddr = 0;
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C] Responding device found at 0x%02X\n", a);
            if (a == GT911_ADDR1 || a == GT911_ADDR2) {
                detectedAddr = a;
            }
        }
    }

    if (detectedAddr == GT911_ADDR2) {
        Serial.println("[UI] Initializing GT911 at address 0x14 (GT911_ADDR2)...");
        ts.begin(GT911_ADDR2);
    } else {
        Serial.println("[UI] Initializing GT911 at address 0x5D (GT911_ADDR1)...");
        ts.begin(GT911_ADDR1);
    }

    ts.setRotation(ROTATION_LEFT);
    gt911Ready = true;

    // Ensure backlight pin stays HIGH
    digitalWrite(27, HIGH);

    Serial.printf("[UI] Display and GT911 Touch initialized (%dx%d)\n", SCREEN_WIDTH, SCREEN_HEIGHT);
}

bool VaultUI::pollGT911(int16_t& x, int16_t& y) {
    if (!gt911Ready) return false;
    ts.read();
    if (ts.isTouched && ts.touches > 0) {
        if (ts.points[0].x > 480 || ts.points[0].y > 320) return false;

        int rawX = 480 - ts.points[0].x;
        int rawY = 320 - ts.points[0].y;
        if (rawX < 0) rawX = 0; if (rawX >= SCREEN_WIDTH) rawX = SCREEN_WIDTH - 1;
        if (rawY < 0) rawY = 0; if (rawY >= SCREEN_HEIGHT) rawY = SCREEN_HEIGHT - 1;
        x = rawX;
        y = rawY;
        Serial.printf("[TOUCH-GT911] Pressed at x=%d y=%d (raw: %d, %d)\n", x, y, ts.points[0].x, ts.points[0].y);
        return true;
    }
    return false;
}

bool VaultUI::getTouch(int16_t& x, int16_t& y) {
    // 1. Physical BOOT button on CYD PCB (GPIO 0) — guaranteed working click!
    if (digitalRead(0) == LOW) {
        delay(150);
        x = SCREEN_WIDTH / 2;
        y = 110; // Default button target
        Serial.println("[INPUT] BOOT button pressed — activating action!");
        return true;
    }

    // 2. GT911 Capacitive Touch (primary for this board)
    return pollGT911(x, y);
}

void VaultUI::setLed(bool r, bool b, bool g) {
    digitalWrite(LED_R, r ? LOW : HIGH);
    digitalWrite(LED_G, g ? LOW : HIGH);
    digitalWrite(LED_B, b ? LOW : HIGH);
}

uint16_t VaultUI::getTierColor(int tier) {
    switch (tier) {
        case 5: return COLOR_TIER5_FORGED;
        case 4: return COLOR_TIER4_QUANTUM;
        case 3: return COLOR_TIER3_REFINED;
        case 2: return COLOR_TIER2_STANDARD;
        default: return COLOR_TIER1_SCRAP;
    }
}

const char* VaultUI::getTierName(int tier) {
    switch (tier) {
        case 5: return "Tier 5 Forged";
        case 4: return "Tier 4 Quantum";
        case 3: return "Tier 3 Refined";
        case 2: return "Tier 2 Standard";
        default: return "Tier 1 Scrap";
    }
}

void VaultUI::drawHeader(const char* title, bool sdOk, bool unlocked, ChainNetwork chain) {
    tft.fillRect(0, 0, SCREEN_WIDTH, 22, COLOR_DARK_GRAY);
    tft.drawFastHLine(0, 22, SCREEN_WIDTH, COLOR_PHOSPHOR);

    tft.setTextColor(COLOR_PHOSPHOR, COLOR_DARK_GRAY);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(title, 6, 4, 2);

    // Network Chain Badge
    tft.setTextDatum(TR_DATUM);
    if (chain == CHAIN_SIDECHAIN_BCAP) {
        tft.setTextColor(COLOR_AMBER, COLOR_DARK_GRAY);
        tft.drawString("[BCAP SIDE]", SCREEN_WIDTH - 120, 4, 2);
    } else {
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_DARK_GRAY);
        tft.drawString("[CAPS MAIN]", SCREEN_WIDTH - 120, 4, 2);
    }

    // SD Badge
    if (sdOk) {
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_DARK_GRAY);
        tft.drawString("[SD:OK]", SCREEN_WIDTH - 65, 4, 2);
    } else {
        tft.setTextColor(COLOR_ALERT_RED, COLOR_DARK_GRAY);
        tft.drawString("[NO-SD]", SCREEN_WIDTH - 65, 4, 2);
    }

    // Vault Status
    if (unlocked) {
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_DARK_GRAY);
        tft.drawString("[ACT]", SCREEN_WIDTH - 6, 4, 2);
    } else {
        tft.setTextColor(COLOR_ALERT_RED, COLOR_DARK_GRAY);
        tft.drawString("[LCK]", SCREEN_WIDTH - 6, 4, 2);
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
    setLed(false, true, false);
    tft.fillScreen(COLOR_BG);

    tft.drawRect(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, COLOR_PHOSPHOR);
    tft.drawRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, COLOR_PHOSPHOR_DIM);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("=== WASTELAND COLD VAULT ===", SCREEN_WIDTH / 2, 16, 4);

    tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
    tft.drawString("CYD-3.5 HARDWARE EDITION (ST7796 480x320)", SCREEN_WIDTH / 2, 46, 2);

    tft.setTextDatum(TL_DATUM);
    int y = 78;
    tft.drawString("> ESP32-D0WDQ6 @ 240MHz: READY", 32, y, 2); y += 22;
    tft.drawString("> CRYPTO ENGINE: secp256k1 Native", 32, y, 2); y += 22;
    tft.drawString("> AIR-GAP: WiFi/BT Radios OFF", 32, y, 2); y += 22;
    tft.drawString("> CHAINS: CAPS Mainnet + BCAP Sidechain", 32, y, 2); y += 22;
    tft.drawString("> TOUCH: GT911 Capacitive Display Engine", 32, y, 2); y += 22;
    tft.drawString("> STORAGE: MicroSD PSBT Engine", 32, y, 2); y += 28;

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("[ TAP SCREEN TO ENTER VAULT ]", SCREEN_WIDTH / 2, y, 4);
}

void VaultUI::showDashboard(const VaultState& state) {
    setLed(false, state.isUnlocked, !state.isUnlocked);
    tft.fillScreen(COLOR_BG);
    drawHeader("CAPS CYD VAULT", state.sdMounted, state.isUnlocked, state.activeChain);

    // Active Address Card Box
    tft.drawRect(12, 30, SCREEN_WIDTH - 24, 50, COLOR_PHOSPHOR_DIM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    char buf[64];
    const char* chainName = (state.activeChain == CHAIN_SIDECHAIN_BCAP) ? "BCAP Sidechain" : "CAPS Mainnet";
    snprintf(buf, sizeof(buf), "%s Address (#%u):", chainName, state.addressIndex);
    tft.drawString(buf, 20, 34, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    if (strlen(state.currentAddress) > 0) {
        tft.drawString(state.currentAddress, 20, 52, 2);
    } else {
        tft.drawString("No address loaded (Locked)", 20, 52, 2);
    }

    // Button 1: Receive & Show QR
    drawButton(12, 88, SCREEN_WIDTH - 24, 44, "1. RECEIVE & SHOW QR CODE", COLOR_PHOSPHOR, COLOR_PHOSPHOR);

    // Button 2: Wasteland Card Gallery
    char galleryLabel[48];
    snprintf(galleryLabel, sizeof(galleryLabel), "2. CARD INVENTORY (%u CARDS)", state.totalCards);
    drawButton(12, 140, SCREEN_WIDTH - 24, 44, galleryLabel, COLOR_TIER4_QUANTUM, COLOR_TIER4_QUANTUM);

    // Button 3: Sign Transaction from SD Card
    drawButton(12, 192, SCREEN_WIDTH - 24, 44, "3. SIGN TRANSACTION (SD CARD)", COLOR_AMBER, COLOR_AMBER);

    // Bottom Row: Chain Switcher & Lock Button
    int bw = (SCREEN_WIDTH - 36) / 2;
    const char* switchLabel = (state.activeChain == CHAIN_MAINNET_CAPS) ? "SWITCH -> BCAP" : "SWITCH -> CAPS";
    drawButton(12, 246, bw, 50, switchLabel, COLOR_PHOSPHOR, COLOR_PHOSPHOR);
    drawButton(24 + bw, 246, bw, 50, "LOCK & WIPE", COLOR_ALERT_RED, COLOR_ALERT_RED);
}

void VaultUI::renderQR(const char* payload, int x, int y, int scale) {
    size_t len = strlen(payload);
    uint8_t version = 3;
    if (len > 120) version = 8;
    else if (len > 80) version = 6;
    else if (len > 50) version = 4;

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(8)];
    qrcode_initText(&qrcode, qrcodeData, version, 0, payload);

    int size = qrcode.size;
    int totalPx = size * scale;

    tft.fillRect(x - 4, y - 4, totalPx + 8, totalPx + 8, COLOR_WHITE);

    for (uint8_t qy = 0; qy < size; qy++) {
        for (uint8_t qx = 0; qx < size; qx++) {
            uint16_t color = qrcode_getModule(&qrcode, qx, qy) ? COLOR_BG : COLOR_WHITE;
            tft.fillRect(x + (qx * scale), y + (qy * scale), scale, scale, color);
        }
    }
}

void VaultUI::showReceiveQR(const char* address, uint32_t index, ChainNetwork chain) {
    tft.fillScreen(COLOR_BG);
    drawHeader("RECEIVE ADDRESS", true, true, chain);

    int qrX = (SCREEN_WIDTH / 2) - 66;
    int qrY = 30;
    renderQR(address, qrX, qrY, 4);

    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    char idxBuf[48];
    uint32_t coinType = (chain == CHAIN_SIDECHAIN_BCAP) ? 1 : 0;
    snprintf(idxBuf, sizeof(idxBuf), "Derivation: m/84'/%u'/0'/0/%u", coinType, index);
    tft.drawString(idxBuf, SCREEN_WIDTH / 2, 178, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString(address, SCREEN_WIDTH / 2, 202, 2);

    int btnW = (SCREEN_WIDTH - 48) / 3;
    drawButton(12, 246, btnW, 48, "< PREV", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
    drawButton(24 + btnW, 246, btnW, 48, "BACK", COLOR_AMBER, COLOR_AMBER);
    drawButton(36 + (btnW * 2), 246, btnW, 48, "NEXT >", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showCardGallery(const CollectibleCard& card, uint32_t currentIdx, uint32_t totalCards) {
    tft.fillScreen(COLOR_BG);
    char head[48];
    snprintf(head, sizeof(head), "CARD GALLERY [%u/%u]", currentIdx + 1, totalCards);
    drawHeader(head, true, true, CHAIN_SIDECHAIN_BCAP);

    uint16_t tierCol = getTierColor(card.tier);

    // Card frame on left (x: 16, y: 32, w: 170, h: 198)
    tft.drawRect(16, 32, 170, 198, tierCol);
    tft.drawRect(17, 33, 168, 196, tierCol);

    bool drawnBmp = false;
    if (strlen(card.imagePath) > 0) {
        drawnBmp = VaultSD.drawCardBMP(card.imagePath, tft, 20, 36, 162, 190);
    }

    if (!drawnBmp) {
        // Fallback vector bottle-cap graphics
        tft.fillCircle(101, 131, 56, tierCol);
        tft.fillCircle(101, 131, 46, COLOR_DARK_GRAY);
        tft.setTextColor(tierCol, COLOR_DARK_GRAY);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("CAP", 101, 131, 4);
    }

    // Card Metadata on right (x: 202..SCREEN_WIDTH-16)
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(tierCol, COLOR_BG);
    tft.drawString(card.title, 202, 34, 4);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    char serBuf[32];
    snprintf(serBuf, sizeof(serBuf), "Serial: #%lld", card.serial);
    tft.drawString(serBuf, 202, 70, 2);

    tft.setTextColor(tierCol, COLOR_BG);
    tft.drawString(getTierName(card.tier), 202, 94, 2);

    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("Standard: AuxPoW / NFT", 202, 120, 2);

    if (strlen(card.recipeHash) > 0) {
        tft.setTextColor(COLOR_PHOSPHOR_DIM, COLOR_BG);
        char hashBuf[32];
        snprintf(hashBuf, sizeof(hashBuf), "Hash: %.14s...", card.recipeHash);
        tft.drawString(hashBuf, 202, 146, 2);
    }

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString("Status: In Cold Vault", 202, 172, 2);

    // Navigation buttons
    int btnW = (SCREEN_WIDTH - 48) / 3;
    drawButton(12, 246, btnW, 48, "< PREV", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
    drawButton(24 + btnW, 246, btnW, 48, "BACK", COLOR_AMBER, COLOR_AMBER);
    drawButton(36 + (btnW * 2), 246, btnW, 48, "NEXT >", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showSignReview(const UnsignedTxDetails& tx, ChainNetwork chain) {
    setLed(false, false, true);
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSACTION REVIEW", true, true, chain);

    tft.drawRect(12, 30, SCREEN_WIDTH - 24, 204, COLOR_AMBER);
    tft.setTextDatum(TL_DATUM);

    if (tx.isCardTrade) {
        // WYSIWYS Card Trade View
        tft.drawRect(20, 38, 120, 140, COLOR_TIER4_QUANTUM);
        if (strlen(tx.tradedCard.imagePath) > 0) {
            VaultSD.drawCardBMP(tx.tradedCard.imagePath, tft, 22, 40, 116, 136);
        } else {
            tft.setTextColor(COLOR_TIER4_QUANTUM, COLOR_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("CARD", 80, 108, 4);
            tft.setTextDatum(TL_DATUM);
        }

        tft.setTextColor(COLOR_TIER4_QUANTUM, COLOR_BG);
        tft.drawString("TRADING POST SWAP", 152, 40, 2);

        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        tft.drawString(tx.tradedCard.title, 152, 64, 4);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("PAYOUT:", 152, 100, 2);
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
        double capsAmt = (double)tx.amountSats / 1e8;
        char amtBuf[32];
        snprintf(amtBuf, sizeof(amtBuf), "%.2f BCAP", capsAmt);
        tft.drawString(amtBuf, 220, 100, 2);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("LISTING FEE: 100.00 BCAP", 152, 124, 2);

        tft.setTextColor(COLOR_ALERT_RED, COLOR_BG);
        tft.drawString("VERIFY CARD BEFORE SIGNING", 20, 192, 2);
    } else {
        // Standard Payment / Send View
        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("PAY TO:", 20, 40, 2);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        tft.drawString(tx.recipient, 20, 60, 2);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("AMOUNT:", 20, 88, 2);
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
        double capsAmt = (double)tx.amountSats / 1e8;
        const char* sym = (chain == CHAIN_SIDECHAIN_BCAP) ? "BCAP" : "CAPS";
        char amtBuf[32];
        snprintf(amtBuf, sizeof(amtBuf), "%.4f %s", capsAmt, sym);
        tft.drawString(amtBuf, 96, 88, 4);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("FEE:", 20, 128, 2);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        double capsFee = (double)tx.feeSats / 1e8;
        char feeBuf[32];
        snprintf(feeBuf, sizeof(feeBuf), "%.6f %s", capsFee, sym);
        tft.drawString(feeBuf, 60, 128, 2);

        char metaBuf[48];
        snprintf(metaBuf, sizeof(metaBuf), "Inputs: %u  |  Outputs: %u", tx.inputCount, tx.outputCount);
        tft.drawString(metaBuf, 20, 154, 2);

        tft.setTextColor(COLOR_ALERT_RED, COLOR_BG);
        tft.drawString("! VERIFY RECIPIENT ON AIR-GAPPED SCREEN !", 20, 192, 2);
    }

    int bw = (SCREEN_WIDTH - 36) / 2;
    drawButton(12, 246, bw, 50, "X REJECT", COLOR_ALERT_RED, COLOR_ALERT_RED);
    drawButton(24 + bw, 246, bw, 50, "V APPROVE & SIGN", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showSignedSuccess(const char* signedBase64, const char* filename) {
    setLed(false, true, false);
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSACTION SIGNED!", true, true, CHAIN_MAINNET_CAPS);

    int qrX = (SCREEN_WIDTH / 2) - 52;
    int qrY = 30;
    renderQR(signedBase64, qrX, qrY, 3);

    tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SAVED TO SD CARD:", SCREEN_WIDTH / 2, 168, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    char buf[64];
    snprintf(buf, sizeof(buf), "signed_%s", filename);
    tft.drawString(buf, SCREEN_WIDTH / 2, 194, 2);

    drawButton(80, 246, SCREEN_WIDTH - 160, 48, "RETURN TO DASHBOARD", COLOR_AMBER, COLOR_AMBER);
}

void VaultUI::showLocked() {
    setLed(true, false, false);
    tft.fillScreen(COLOR_BG);
    drawHeader("VAULT LOCKED", false, false, CHAIN_MAINNET_CAPS);

    tft.drawRect(12, 34, SCREEN_WIDTH - 24, 196, COLOR_ALERT_RED);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_ALERT_RED, COLOR_BG);
    tft.drawString("MEMORY ZEROIZED & LOCKED", SCREEN_WIDTH / 2, 58, 4);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString("All private keys and seeds have been", SCREEN_WIDTH / 2, 102, 2);
    tft.drawString("scrubbed from RAM buffers.", SCREEN_WIDTH / 2, 126, 2);
    tft.drawString("No keys exist on flash storage.", SCREEN_WIDTH / 2, 150, 2);

    drawButton(80, 246, SCREEN_WIDTH - 160, 48, "INITIALIZE / UNLOCK", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showMessage(const char* title, const char* msg, uint16_t color) {
    tft.fillRect(30, 80, SCREEN_WIDTH - 60, 140, COLOR_BG);
    tft.drawRect(30, 80, SCREEN_WIDTH - 60, 140, color);
    tft.drawRect(32, 82, SCREEN_WIDTH - 64, 136, color);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(color, COLOR_BG);
    tft.drawString(title, SCREEN_WIDTH / 2, 98, 4);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString(msg, SCREEN_WIDTH / 2, 150, 2);
}
