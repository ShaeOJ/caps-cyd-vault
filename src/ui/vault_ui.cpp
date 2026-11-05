#include "vault_ui.h"
#include <SPI.h>
#include "../storage/sd_vault.h"

VaultUI UI;

VaultUI::VaultUI() : ts(TOUCH_CS, TOUCH_IRQ) {
}

void VaultUI::begin() {
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    // CYD RGB LEDs are active LOW — HIGH turns them off
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, HIGH);

    // Turn on backlight (GPIO 21 on 1-USB CYD, GPIO 27 on 2-USB CYD)
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    tft.init();
    tft.setRotation(1); // 320x240 landscape
    tft.fillScreen(COLOR_BG);

    SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin();
    ts.setRotation(1);

    Serial.println("[UI] Display and Touch initialized (320x240)");
}

void VaultUI::setLed(bool r, bool g, bool b) {
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

bool VaultUI::getTouch(int16_t& x, int16_t& y) {
    if (!ts.touched()) return false;
    TS_Point p = ts.getPoint();

    if (p.z < 200) return false;

    int16_t mappedX = map(p.x, 200, 3800, 0, SCREEN_WIDTH);
    int16_t mappedY = map(p.y, 250, 3850, 0, SCREEN_HEIGHT);

    if (mappedX < 0) mappedX = 0;
    if (mappedX >= SCREEN_WIDTH) mappedX = SCREEN_WIDTH - 1;
    if (mappedY < 0) mappedY = 0;
    if (mappedY >= SCREEN_HEIGHT) mappedY = SCREEN_HEIGHT - 1;

    x = mappedX;
    y = mappedY;
    return true;
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
    tft.drawString("=== WASTELAND COLD VAULT ===", SCREEN_WIDTH / 2, 16, 2);

    tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
    tft.drawString("CYD-2.8 HARDWARE EDITION", SCREEN_WIDTH / 2, 38, 2);

    tft.setTextDatum(TL_DATUM);
    int y = 66;
    tft.drawString("> ESP32-D0WDQ6 @ 240MHz: READY", 16, y, 2); y += 19;
    tft.drawString("> CRYPTO ENGINE: secp256k1 Native", 16, y, 2); y += 19;
    tft.drawString("> AIR-GAP: WiFi/BT Radios OFF", 16, y, 2); y += 19;
    tft.drawString("> CHAINS: CAPS Mainnet + BCAP Sidechain", 16, y, 2); y += 19;
    tft.drawString("> GALLERY: Wasteland Cards (16-bit BMP)", 16, y, 2); y += 19;
    tft.drawString("> STORAGE: MicroSD PSBT Engine", 16, y, 2); y += 24;

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("[ TAP SCREEN TO ENTER VAULT ]", SCREEN_WIDTH / 2, y, 2);
}

void VaultUI::showDashboard(const VaultState& state) {
    setLed(false, state.isUnlocked, !state.isUnlocked);
    tft.fillScreen(COLOR_BG);
    drawHeader("CAPS CYD VAULT", state.sdMounted, state.isUnlocked, state.activeChain);

    // Active Address Card Box
    tft.drawRect(8, 28, SCREEN_WIDTH - 16, 44, COLOR_PHOSPHOR_DIM);
    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    char buf[64];
    const char* chainName = (state.activeChain == CHAIN_SIDECHAIN_BCAP) ? "BCAP Sidechain" : "CAPS Mainnet";
    snprintf(buf, sizeof(buf), "%s Address (#%u):", chainName, state.addressIndex);
    tft.drawString(buf, 14, 32, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    if (strlen(state.currentAddress) > 0) {
        tft.drawString(state.currentAddress, 14, 48, 2);
    } else {
        tft.drawString("No address loaded (Locked)", 14, 48, 2);
    }

    // Button 1: Receive & Show QR
    drawButton(8, 76, SCREEN_WIDTH - 16, 34, "1. RECEIVE & SHOW QR CODE", COLOR_PHOSPHOR, COLOR_PHOSPHOR);

    // Button 2: Wasteland Card Gallery
    char galleryLabel[48];
    snprintf(galleryLabel, sizeof(galleryLabel), "2. CARD INVENTORY (%u CARDS)", state.totalCards);
    drawButton(8, 114, SCREEN_WIDTH - 16, 34, galleryLabel, COLOR_TIER4_QUANTUM, COLOR_TIER4_QUANTUM);

    // Button 3: Sign Transaction from SD Card
    drawButton(8, 152, SCREEN_WIDTH - 16, 34, "3. SIGN TRANSACTION (SD CARD)", COLOR_AMBER, COLOR_AMBER);

    // Bottom Row: Chain Switcher & Lock Button
    const char* switchLabel = (state.activeChain == CHAIN_MAINNET_CAPS) ? "SWITCH -> BCAP" : "SWITCH -> CAPS";
    drawButton(8, 192, 148, 38, switchLabel, COLOR_PHOSPHOR, COLOR_PHOSPHOR);
    drawButton(164, 192, 148, 38, "LOCK & WIPE", COLOR_ALERT_RED, COLOR_ALERT_RED);
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

    int qrX = (SCREEN_WIDTH / 2) - 52;
    int qrY = 30;
    renderQR(address, qrX, qrY, 3);

    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    char idxBuf[48];
    uint32_t coinType = (chain == CHAIN_SIDECHAIN_BCAP) ? 1 : 0;
    snprintf(idxBuf, sizeof(idxBuf), "Derivation: m/84'/%u'/0'/0/%u", coinType, index);
    tft.drawString(idxBuf, SCREEN_WIDTH / 2, 142, 2);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString(address, SCREEN_WIDTH / 2, 162, 2);

    drawButton(8, 196, 96, 36, "< PREV", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
    drawButton(112, 196, 96, 36, "BACK", COLOR_AMBER, COLOR_AMBER);
    drawButton(216, 196, 96, 36, "NEXT >", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showCardGallery(const CollectibleCard& card, uint32_t currentIdx, uint32_t totalCards) {
    tft.fillScreen(COLOR_BG);
    char head[48];
    snprintf(head, sizeof(head), "CARD GALLERY [%u/%u]", currentIdx + 1, totalCards);
    drawHeader(head, true, true, CHAIN_SIDECHAIN_BCAP);

    uint16_t tierCol = getTierColor(card.tier);

    // Card frame on left (x: 10, y: 30, w: 130, h: 155)
    tft.drawRect(10, 30, 130, 155, tierCol);
    tft.drawRect(11, 31, 128, 153, tierCol);

    bool drawnBmp = false;
    if (strlen(card.imagePath) > 0) {
        drawnBmp = VaultSD.drawCardBMP(card.imagePath, tft, 14, 34, 122, 147);
    }

    if (!drawnBmp) {
        // Fallback vector bottle-cap graphics
        tft.fillCircle(75, 107, 44, tierCol);
        tft.fillCircle(75, 107, 36, COLOR_DARK_GRAY);
        tft.setTextColor(tierCol, COLOR_DARK_GRAY);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("CAP", 75, 107, 4);
    }

    // Card Metadata on right (x: 150..310)
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(tierCol, COLOR_BG);
    tft.drawString(card.title, 150, 34, 4);

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    char serBuf[32];
    snprintf(serBuf, sizeof(serBuf), "Serial: #%lld", card.serial);
    tft.drawString(serBuf, 150, 68, 2);

    tft.setTextColor(tierCol, COLOR_BG);
    tft.drawString(getTierName(card.tier), 150, 88, 2);

    tft.setTextColor(COLOR_AMBER, COLOR_BG);
    tft.drawString("Standard: AuxPoW / NFT", 150, 110, 2);

    if (strlen(card.recipeHash) > 0) {
        tft.setTextColor(COLOR_PHOSPHOR_DIM, COLOR_BG);
        char hashBuf[32];
        snprintf(hashBuf, sizeof(hashBuf), "Hash: %.10s...", card.recipeHash);
        tft.drawString(hashBuf, 150, 132, 2);
    }

    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.drawString("Status: In Cold Vault", 150, 154, 2);

    // Navigation buttons
    drawButton(8, 196, 96, 36, "< PREV", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
    drawButton(112, 196, 96, 36, "BACK", COLOR_AMBER, COLOR_AMBER);
    drawButton(216, 196, 96, 36, "NEXT >", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showSignReview(const UnsignedTxDetails& tx, ChainNetwork chain) {
    setLed(false, false, true);
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSACTION REVIEW", true, true, chain);

    tft.drawRect(6, 28, SCREEN_WIDTH - 12, 154, COLOR_AMBER);
    tft.setTextDatum(TL_DATUM);

    if (tx.isCardTrade) {
        // WYSIWYS Card Trade View
        tft.drawRect(10, 32, 90, 110, COLOR_TIER4_QUANTUM);
        if (strlen(tx.tradedCard.imagePath) > 0) {
            VaultSD.drawCardBMP(tx.tradedCard.imagePath, tft, 12, 34, 86, 106);
        } else {
            tft.setTextColor(COLOR_TIER4_QUANTUM, COLOR_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("CARD", 55, 87, 2);
            tft.setTextDatum(TL_DATUM);
        }

        tft.setTextColor(COLOR_TIER4_QUANTUM, COLOR_BG);
        tft.drawString("TRADING POST SWAP", 108, 34, 2);

        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        tft.drawString(tx.tradedCard.title, 108, 52, 2);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("PAYOUT:", 108, 72, 2);
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
        double capsAmt = (double)tx.amountSats / 1e8;
        char amtBuf[32];
        snprintf(amtBuf, sizeof(amtBuf), "%.2f BCAP", capsAmt);
        tft.drawString(amtBuf, 168, 72, 2);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("LISTING FEE: 100.00 BCAP", 108, 92, 2);

        tft.setTextColor(COLOR_ALERT_RED, COLOR_BG);
        tft.drawString("VERIFY CARD BEFORE SIGNING", 12, 152, 2);
    } else {
        // Standard Payment / Send View
        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("PAY TO:", 12, 34, 2);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        tft.drawString(tx.recipient, 12, 52, 2);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("AMOUNT:", 12, 74, 2);
        tft.setTextColor(COLOR_PHOSPHOR, COLOR_BG);
        double capsAmt = (double)tx.amountSats / 1e8;
        const char* sym = (chain == CHAIN_SIDECHAIN_BCAP) ? "BCAP" : "CAPS";
        char amtBuf[32];
        snprintf(amtBuf, sizeof(amtBuf), "%.4f %s", capsAmt, sym);
        tft.drawString(amtBuf, 86, 74, 4);

        tft.setTextColor(COLOR_AMBER, COLOR_BG);
        tft.drawString("FEE:", 12, 108, 2);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        double capsFee = (double)tx.feeSats / 1e8;
        char feeBuf[32];
        snprintf(feeBuf, sizeof(feeBuf), "%.6f %s", capsFee, sym);
        tft.drawString(feeBuf, 50, 108, 2);

        char metaBuf[48];
        snprintf(metaBuf, sizeof(metaBuf), "Inputs: %u  |  Outputs: %u", tx.inputCount, tx.outputCount);
        tft.drawString(metaBuf, 12, 130, 2);

        tft.setTextColor(COLOR_ALERT_RED, COLOR_BG);
        tft.drawString("! VERIFY RECIPIENT ON AIR-GAPPED SCREEN !", 12, 152, 2);
    }

    drawButton(8, 190, 148, 42, "X REJECT", COLOR_ALERT_RED, COLOR_ALERT_RED);
    drawButton(164, 190, 148, 42, "V APPROVE & SIGN", COLOR_PHOSPHOR, COLOR_PHOSPHOR);
}

void VaultUI::showSignedSuccess(const char* signedBase64, const char* filename) {
    setLed(false, true, false);
    tft.fillScreen(COLOR_BG);
    drawHeader("TRANSACTION SIGNED!", true, true, CHAIN_MAINNET_CAPS);

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
    setLed(true, false, false);
    tft.fillScreen(COLOR_BG);
    drawHeader("VAULT LOCKED", false, false, CHAIN_MAINNET_CAPS);

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
