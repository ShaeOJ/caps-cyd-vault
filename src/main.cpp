#include <Arduino.h>
#include "../include/config.h"
#include "../include/vault_types.h"
#include "crypto/caps_crypto.h"
#include "storage/sd_vault.h"
#include "ui/vault_ui.h"

static VaultState state;
static String rawUnsignedPSBT = "";
static CollectibleCard cards[MAX_GALLERY_CARDS];

static const char* DEFAULT_MNEMONIC = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";

void refreshVaultAddress() {
    if (VaultCrypto.isUnlocked()) {
        String addr = VaultCrypto.getAddress(state.addressIndex, state.activeChain);
        strncpy(state.currentAddress, addr.c_str(), sizeof(state.currentAddress) - 1);
    } else {
        state.currentAddress[0] = '\0';
    }
}

void loadCardsFromSD() {
    if (state.sdMounted) {
        state.totalCards = VaultSD.scanCards(cards, MAX_GALLERY_CARDS);
        state.currentCardIndex = 0;
    } else {
        state.totalCards = 0;
    }
}

void unlockVault() {
    String seedMnemonic = DEFAULT_MNEMONIC;
    if (VaultSD.isMounted()) {
        File f = SD.open("/seed.txt", FILE_READ);
        if (f && f.size() > 10) {
            seedMnemonic = f.readString();
            seedMnemonic.trim();
            f.close();
            Serial.println("[VAULT] Loaded custom seed from /seed.txt on SD card");
        }
    }

    if (VaultCrypto.initFromMnemonic(seedMnemonic)) {
        state.isUnlocked = true;
        refreshVaultAddress();
        Serial.printf("[VAULT] Unlocked! Active address: %s\n", state.currentAddress);
    } else {
        Serial.println("[VAULT] Failed to initialize mnemonic!");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n========================================");
    Serial.println("  CAPS CYD VAULT — Cold Hardware Wallet ");
    Serial.println("  Target: ESP32-2432S028R (CYD 2.8)     ");
    Serial.println("  Chains: CAPS Mainnet + BCAP Sidechain ");
    Serial.println("========================================");

    UI.begin();
    state.sdMounted = VaultSD.begin();
    loadCardsFromSD();
    unlockVault();

    state.currentScreen = SCREEN_SPLASH;
    UI.showSplash();
}

void loop() {
    int16_t tx = 0, ty = 0;
    if (UI.getTouch(tx, ty)) {
        delay(180);

        switch (state.currentScreen) {
            case SCREEN_SPLASH: {
                state.currentScreen = SCREEN_DASHBOARD;
                UI.showDashboard(state);
                break;
            }

            case SCREEN_DASHBOARD: {
                // Button 1: Show Receive QR Code (y: 76..110)
                if (ty >= 76 && ty <= 110) {
                    state.currentScreen = SCREEN_RECEIVE_QR;
                    UI.showReceiveQR(state.currentAddress, state.addressIndex, state.activeChain);
                }
                // Button 2: Card Inventory / Gallery (y: 114..148)
                else if (ty >= 114 && ty <= 148) {
                    if (state.totalCards > 0) {
                        state.currentScreen = SCREEN_CARD_GALLERY;
                        UI.showCardGallery(cards[state.currentCardIndex], state.currentCardIndex, state.totalCards);
                    } else {
                        // Scan cards again or show notice
                        loadCardsFromSD();
                        if (state.totalCards > 0) {
                            state.currentScreen = SCREEN_CARD_GALLERY;
                            UI.showCardGallery(cards[state.currentCardIndex], state.currentCardIndex, state.totalCards);
                        } else {
                            UI.showMessage("EMPTY INVENTORY", "Place .bmp cards in /cards/ on SD", COLOR_AMBER);
                            delay(2000);
                            UI.showDashboard(state);
                        }
                    }
                }
                // Button 3: Sign Transaction from SD (y: 152..186)
                else if (ty >= 152 && ty <= 186) {
                    if (!state.sdMounted && !VaultSD.begin()) {
                        UI.showMessage("NO SD CARD", "Insert MicroSD with .psbt file", COLOR_ALERT_RED);
                        delay(1500);
                        UI.showDashboard(state);
                        break;
                    }

                    String filename, content;
                    UI.showMessage("SCANNING SD", "Looking for .psbt file...", COLOR_AMBER);
                    if (VaultSD.findUnsignedFile(filename, content)) {
                        rawUnsignedPSBT = content;
                        strncpy(state.pendingTx.filename, filename.c_str(), sizeof(state.pendingTx.filename) - 1);
                        if (VaultCrypto.parsePSBTFile(content, state.pendingTx, state.activeChain)) {
                            state.currentScreen = SCREEN_SIGN_REVIEW;
                            UI.showSignReview(state.pendingTx, state.activeChain);
                        } else {
                            UI.showMessage("INVALID PSBT", "File could not be parsed", COLOR_ALERT_RED);
                            delay(2000);
                            UI.showDashboard(state);
                        }
                    } else {
                        UI.showMessage("NO PSBT FOUND", "Place tx.psbt on SD card", COLOR_AMBER);
                        delay(2000);
                        UI.showDashboard(state);
                    }
                }
                // Bottom Row Left: Switch Network Chain (x: 8..156, y: 192..230)
                else if (tx >= 8 && tx <= 156 && ty >= 192 && ty <= 230) {
                    if (state.activeChain == CHAIN_MAINNET_CAPS) {
                        state.activeChain = CHAIN_SIDECHAIN_BCAP;
                    } else {
                        state.activeChain = CHAIN_MAINNET_CAPS;
                    }
                    refreshVaultAddress();
                    UI.showDashboard(state);
                }
                // Bottom Row Right: Lock Vault & Wipe Keys (x: 164..312, y: 192..230)
                else if (tx >= 164 && tx <= 312 && ty >= 192 && ty <= 230) {
                    VaultCrypto.wipe();
                    state.isUnlocked = false;
                    state.currentAddress[0] = '\0';
                    state.currentScreen = SCREEN_LOCKED;
                    UI.showLocked();
                }
                break;
            }

            case SCREEN_CARD_GALLERY: {
                // Navigation buttons at y: 196..234
                if (ty >= 196 && ty <= 234) {
                    // < PREV (x: 8..104)
                    if (tx >= 8 && tx <= 104) {
                        if (state.totalCards > 0) {
                            if (state.currentCardIndex > 0) {
                                state.currentCardIndex--;
                            } else {
                                state.currentCardIndex = state.totalCards - 1; // Wrap around
                            }
                            UI.showCardGallery(cards[state.currentCardIndex], state.currentCardIndex, state.totalCards);
                        }
                    }
                    // BACK (x: 112..208)
                    else if (tx >= 112 && tx <= 208) {
                        state.currentScreen = SCREEN_DASHBOARD;
                        UI.showDashboard(state);
                    }
                    // NEXT > (x: 216..312)
                    else if (tx >= 216 && tx <= 312) {
                        if (state.totalCards > 0) {
                            if (state.currentCardIndex + 1 < state.totalCards) {
                                state.currentCardIndex++;
                            } else {
                                state.currentCardIndex = 0; // Wrap around
                            }
                            UI.showCardGallery(cards[state.currentCardIndex], state.currentCardIndex, state.totalCards);
                        }
                    }
                }
                break;
            }

            case SCREEN_RECEIVE_QR: {
                if (ty >= 196 && ty <= 232) {
                    if (tx >= 8 && tx <= 104) {
                        if (state.addressIndex > 0) {
                            state.addressIndex--;
                            refreshVaultAddress();
                            UI.showReceiveQR(state.currentAddress, state.addressIndex, state.activeChain);
                        }
                    } else if (tx >= 112 && tx <= 208) {
                        state.currentScreen = SCREEN_DASHBOARD;
                        UI.showDashboard(state);
                    } else if (tx >= 216 && tx <= 312) {
                        state.addressIndex++;
                        refreshVaultAddress();
                        UI.showReceiveQR(state.currentAddress, state.addressIndex, state.activeChain);
                    }
                }
                break;
            }

            case SCREEN_SIGN_REVIEW: {
                if (ty >= 190 && ty <= 232) {
                    if (tx >= 8 && tx <= 156) {
                        rawUnsignedPSBT = "";
                        state.currentScreen = SCREEN_DASHBOARD;
                        UI.showDashboard(state);
                    } else if (tx >= 164 && tx <= 312) {
                        UI.showMessage("SIGNING...", "Generating ECDSA secp256k1", COLOR_PHOSPHOR);
                        String signedBase64, errorMsg;
                        if (VaultCrypto.signPSBT(rawUnsignedPSBT, signedBase64, errorMsg)) {
                            VaultSD.saveSignedPSBT(state.pendingTx.filename, signedBase64);
                            state.currentScreen = SCREEN_SIGNED_SUCCESS;
                            UI.showSignedSuccess(signedBase64.c_str(), state.pendingTx.filename);
                        } else {
                            UI.showMessage("SIGN FAILED", errorMsg.c_str(), COLOR_ALERT_RED);
                            delay(2500);
                            state.currentScreen = SCREEN_DASHBOARD;
                            UI.showDashboard(state);
                        }
                    }
                }
                break;
            }

            case SCREEN_SIGNED_SUCCESS: {
                if (ty >= 194 && ty <= 232) {
                    rawUnsignedPSBT = "";
                    state.currentScreen = SCREEN_DASHBOARD;
                    UI.showDashboard(state);
                }
                break;
            }

            case SCREEN_LOCKED: {
                if (ty >= 190 && ty <= 230) {
                    unlockVault();
                    state.currentScreen = SCREEN_DASHBOARD;
                    UI.showDashboard(state);
                }
                break;
            }
        }
    }

    delay(20);
}
