#include <Arduino.h>
#include "config.h"
#include "vault_types.h"
#include "crypto/caps_crypto.h"
#include "storage/sd_vault.h"
#include "ui/vault_ui.h"

static VaultState state;
static String rawUnsignedPSBT = "";

// Default demo mnemonic for initial testing (BIP-39 test seed)
// Users can provide their own seed via /seed.txt on the MicroSD card.
static const char* DEFAULT_MNEMONIC = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";

void refreshVaultAddress() {
    if (VaultCrypto.isUnlocked()) {
        String addr = VaultCrypto.getAddress(state.addressIndex);
        strncpy(state.currentAddress, addr.c_str(), sizeof(state.currentAddress) - 1);
    } else {
        state.currentAddress[0] = '\0';
    }
}

void unlockVault() {
    // Check if custom seed exists on SD card
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
    Serial.println("========================================");

    // 1. Initialize Display & Touch UI
    UI.begin();

    // 2. Initialize MicroSD Card
    state.sdMounted = VaultSD.begin();

    // 3. Unlock initial vault
    unlockVault();

    // 4. Show Splash
    state.currentScreen = SCREEN_SPLASH;
    UI.showSplash();
}

void loop() {
    int16_t tx = 0, ty = 0;
    if (UI.getTouch(tx, ty)) {
        // Debounce touch
        delay(180);

        switch (state.currentScreen) {
            case SCREEN_SPLASH: {
                state.currentScreen = SCREEN_DASHBOARD;
                UI.showDashboard(state);
                break;
            }

            case SCREEN_DASHBOARD: {
                // Button 1: Show Receive QR Code (y: 96..134)
                if (ty >= 96 && ty <= 134) {
                    state.currentScreen = SCREEN_RECEIVE_QR;
                    UI.showReceiveQR(state.currentAddress, state.addressIndex);
                }
                // Button 2: Sign Transaction from SD (y: 142..180)
                else if (ty >= 142 && ty <= 180) {
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
                        if (VaultCrypto.parsePSBTFile(content, state.pendingTx)) {
                            state.currentScreen = SCREEN_SIGN_REVIEW;
                            UI.showSignReview(state.pendingTx);
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
                // Button 3: Lock Vault & Wipe Keys (y: 188..226)
                else if (ty >= 188 && ty <= 226) {
                    VaultCrypto.wipe();
                    state.isUnlocked = false;
                    state.currentAddress[0] = '\0';
                    state.currentScreen = SCREEN_LOCKED;
                    UI.showLocked();
                }
                break;
            }

            case SCREEN_RECEIVE_QR: {
                // Navigation buttons at y: 196..232
                if (ty >= 196 && ty <= 232) {
                    // < PREV (x: 8..104)
                    if (tx >= 8 && tx <= 104) {
                        if (state.addressIndex > 0) {
                            state.addressIndex--;
                            refreshVaultAddress();
                            UI.showReceiveQR(state.currentAddress, state.addressIndex);
                        }
                    }
                    // BACK (x: 112..208)
                    else if (tx >= 112 && tx <= 208) {
                        state.currentScreen = SCREEN_DASHBOARD;
                        UI.showDashboard(state);
                    }
                    // NEXT > (x: 216..312)
                    else if (tx >= 216 && tx <= 312) {
                        state.addressIndex++;
                        refreshVaultAddress();
                        UI.showReceiveQR(state.currentAddress, state.addressIndex);
                    }
                }
                break;
            }

            case SCREEN_SIGN_REVIEW: {
                // Action buttons at y: 190..232
                if (ty >= 190 && ty <= 232) {
                    // REJECT (x: 8..156)
                    if (tx >= 8 && tx <= 156) {
                        rawUnsignedPSBT = "";
                        state.currentScreen = SCREEN_DASHBOARD;
                        UI.showDashboard(state);
                    }
                    // APPROVE & SIGN (x: 164..312)
                    else if (tx >= 164 && tx <= 312) {
                        UI.showMessage("SIGNING...", "Generating ECDSA secp256k1", COLOR_PHOSPHOR);
                        String signedBase64, errorMsg;
                        if (VaultCrypto.signPSBT(rawUnsignedPSBT, signedBase64, errorMsg)) {
                            // Save to SD Card
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
                // Button: Return to Dashboard (y: 194..232)
                if (ty >= 194 && ty <= 232) {
                    rawUnsignedPSBT = "";
                    state.currentScreen = SCREEN_DASHBOARD;
                    UI.showDashboard(state);
                }
                break;
            }

            case SCREEN_LOCKED: {
                // Button: Initialize / Unlock (y: 190..230)
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
