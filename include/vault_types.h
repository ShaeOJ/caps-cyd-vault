#pragma once
#include <Arduino.h>

enum VaultScreen {
    SCREEN_SPLASH,
    SCREEN_DASHBOARD,
    SCREEN_RECEIVE_QR,
    SCREEN_SIGN_REVIEW,
    SCREEN_SIGNED_SUCCESS,
    SCREEN_LOCKED
};

struct UnsignedTxDetails {
    bool valid = false;
    char recipient[72] = "";
    uint64_t amountSats = 0;
    uint64_t feeSats = 0;
    uint32_t inputCount = 0;
    uint32_t outputCount = 0;
    char rawHex[4096] = "";
    char filename[64] = "";
};

struct VaultState {
    VaultScreen currentScreen = SCREEN_SPLASH;
    bool isUnlocked = false;
    bool sdMounted = false;
    uint32_t addressIndex = 0;
    char currentAddress[72] = "";
    UnsignedTxDetails pendingTx;
    char lastSignedHex[4096] = "";
};
