#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include <qrcode.h>

#include "../../include/config.h"
#include "../../include/vault_types.h"

enum TouchMode {
    TOUCH_UNKNOWN = 0,
    TOUCH_GT911 = 1,
    TOUCH_CST816 = 2,
    TOUCH_CHSC6X = 3,
    TOUCH_XPT2046 = 4
};

class VaultUI {
public:
    VaultUI();
    void begin();

    // Touch
    bool getTouch(int16_t& x, int16_t& y);

    // Screen Renderers
    void showSplash();
    void showDashboard(const VaultState& state);
    void showReceiveQR(const char* address, uint32_t index, ChainNetwork chain);
    void showSignReview(const UnsignedTxDetails& tx, ChainNetwork chain);
    void showSignedSuccess(const char* signedBase64, const char* filename);
    void showCardGallery(const CollectibleCard& card, uint32_t currentIdx, uint32_t totalCards);
    void showLocked();
    void showMessage(const char* title, const char* msg, uint16_t color = COLOR_PHOSPHOR);

    // QR Helper
    void renderQR(const char* payload, int x, int y, int scale = 3);

    // LED Status
    void setLed(bool r, bool g, bool b);

    // TFT Direct Access (for BMP rendering)
    TFT_eSPI& getTft() { return tft; }

private:
    TFT_eSPI tft;
    TAMC_GT911 ts;
    bool gt911Ready = false;

    bool pollGT911(int16_t& x, int16_t& y);
    void drawHeader(const char* title, bool sdOk, bool unlocked, ChainNetwork chain);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t borderColor, uint16_t textColor);
    uint16_t getTierColor(int tier);
    const char* getTierName(int tier);
};

extern VaultUI UI;
