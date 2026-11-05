#pragma once
#include <Arduino.h>

#if __has_include("vault_types.h")
#include "config.h"
#include "vault_types.h"
#else
#include "../../include/config.h"
#include "../../include/vault_types.h"
#endif

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
    String getAddress(uint32_t index = 0, ChainNetwork chain = CHAIN_MAINNET_CAPS, uint32_t chainBranch = 0);
    String getAccountXpub(ChainNetwork chain = CHAIN_MAINNET_CAPS);

    // PSBT & Transaction Processing
    bool parsePSBTFile(const String& base64Content, UnsignedTxDetails& outDetails, ChainNetwork currentChain = CHAIN_MAINNET_CAPS);
    bool signPSBT(const String& base64Content, String& signedBase64, String& errorMsg);

    // Helpers
    static String formatSats(uint64_t sats, ChainNetwork chain = CHAIN_MAINNET_CAPS);
    static uint64_t capsToSats(double caps);
    static double satsToCaps(uint64_t sats);

private:
    HDPrivateKey* masterKey = nullptr;
    bool unlocked = false;

    String encodeSegwitAddress(const uint8_t* pubKeyHash20, const char* hrp);
};

extern CapsCrypto VaultCrypto;
