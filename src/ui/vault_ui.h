#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Bitbang.h>
#include <qrcode.h>

#include "../../include/config.h"
#include "../../include/vault_types.h"

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
    XPT2046_Bitbang ts;

    void drawHeader(const char* title, bool sdOk, bool unlocked, ChainNetwork chain);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t borderColor, uint16_t textColor);
    uint16_t getTierColor(int tier);
    const char* getTierName(int tier);
};

extern VaultUI UI;
