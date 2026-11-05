#pragma once
#include <Arduino.h>

enum ChainNetwork {
    CHAIN_MAINNET_CAPS,
    CHAIN_SIDECHAIN_BCAP
};

enum VaultScreen {
    SCREEN_SPLASH,
    SCREEN_DASHBOARD,
    SCREEN_RECEIVE_QR,
    SCREEN_SIGN_REVIEW,
    SCREEN_SIGNED_SUCCESS,
    SCREEN_CARD_GALLERY,
    SCREEN_LOCKED
};

struct CollectibleCard {
    int64_t serial = 0;
    int tier = 1;
    char title[48] = "";
    char recipeHash[65] = "";
    char imagePath[64] = "";
    char jsonPath[64] = "";
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
    bool isCardTrade = false;
    CollectibleCard tradedCard;
};

struct VaultState {
    VaultScreen currentScreen = SCREEN_SPLASH;
    ChainNetwork activeChain = CHAIN_MAINNET_CAPS;
    bool isUnlocked = false;
    bool sdMounted = false;
    uint32_t addressIndex = 0;
    char currentAddress[72] = "";
    UnsignedTxDetails pendingTx;
    char lastSignedHex[4096] = "";

    // Card Gallery
    uint32_t currentCardIndex = 0;
    uint32_t totalCards = 0;
};
