#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <qrcode.h>
#include "config.h"
#include "vault_types.h"

class VaultUI {
public:
    VaultUI();
    void begin();

    // Touch
    bool getTouch(int16_t& x, int16_t& y);

    // Screen Renderers
    void showSplash();
    void showDashboard(const VaultState& state);
    void showReceiveQR(const char* address, uint32_t index);
    void showSignReview(const UnsignedTxDetails& tx);
    void showSignedSuccess(const char* signedBase64, const char* filename);
    void showLocked();
    void showMessage(const char* title, const char* msg, uint16_t color = COLOR_PHOSPHOR);

    // QR Helper
    void renderQR(const char* payload, int x, int y, int scale = 3);

    // LED Status
    void setLed(bool r, bool g, bool b);

private:
    TFT_eSPI tft;
    SPIClass touchSpi;
    XPT2046_Touchscreen ts;

    void drawHeader(const char* title, bool sdOk, bool unlocked);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t borderColor, uint16_t textColor);
};

extern VaultUI UI;
