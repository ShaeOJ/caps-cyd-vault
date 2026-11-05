#pragma once
#include <Arduino.h>
#include "config.h"
#include "vault_types.h"
#include <Bitcoin.h>
#include <PSBT.h>

class CapsCrypto {
public:
    CapsCrypto();
    ~CapsCrypto();

    // Key Management
    bool initFromMnemonic(const String& mnemonic, const String& passphrase = "");
    bool isUnlocked() const;
    void wipe();

    // Address Derivation
    String getAddress(uint32_t index = 0, uint32_t chain = 0, bool regtest = false);
    String getAccountXpub(bool regtest = false);

    // PSBT & Transaction Processing
    bool parsePSBTFile(const String& base64Content, UnsignedTxDetails& outDetails);
    bool signPSBT(const String& base64Content, String& signedBase64, String& errorMsg);

    // Helpers
    static String formatSats(uint64_t sats);
    static uint64_t capsToSats(double caps);
    static double satsToCaps(uint64_t sats);

private:
    HDPrivateKey* masterKey = nullptr;
    bool unlocked = false;

    String encodeSegwitAddress(const uint8_t* pubKeyHash20, bool regtest);
};

extern CapsCrypto VaultCrypto;
