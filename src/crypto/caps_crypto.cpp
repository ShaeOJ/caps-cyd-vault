#include "caps_crypto.h"
#include <Hash.h>
#include <utility/segwit_addr.h>
#include <utility/trezor/memzero.h>

CapsCrypto VaultCrypto;

CapsCrypto::CapsCrypto() {
    masterKey = nullptr;
    unlocked = false;
}

CapsCrypto::~CapsCrypto() {
    wipe();
}

bool CapsCrypto::initFromMnemonic(const String& mnemonic, const String& passphrase) {
    wipe();
    masterKey = new HDPrivateKey();
    int res = masterKey->fromMnemonic(mnemonic, passphrase, &Mainnet);
    if (res <= 0 || !masterKey->isValid()) {
        wipe();
        return false;
    }
    unlocked = true;
    return true;
}

bool CapsCrypto::isUnlocked() const {
    return unlocked && masterKey != nullptr && masterKey->isValid();
}

void CapsCrypto::wipe() {
    if (masterKey != nullptr) {
        memzero(masterKey, sizeof(HDPrivateKey));
        delete masterKey;
        masterKey = nullptr;
    }
    unlocked = false;
}

String CapsCrypto::encodeSegwitAddress(const uint8_t* pubKeyHash20, bool regtest) {
    char addr[80] = {0};
    const char* hrp = regtest ? CAPS_REGTEST_HRP : CAPS_HRP;
    segwit_addr_encode(addr, hrp, 0, pubKeyHash20, 20);
    return String(addr);
}

String CapsCrypto::getAddress(uint32_t index, uint32_t chain, bool regtest) {
    if (!isUnlocked()) return "";

    // BIP84: m/84'/0'/0'/chain/index
    char path[64];
    snprintf(path, sizeof(path), "m/84'/%d'/0'/%d/%d", CAPS_COINTYPE, chain, index);
    HDPrivateKey child = masterKey->derive(path);
    if (!child.isValid()) return "";

    PublicKey pub = child.publicKey();
    uint8_t secArr[65] = {0};
    int secLen = pub.sec(secArr, sizeof(secArr));
    
    uint8_t hash20[20] = {0};
    hash160(secArr, secLen, hash20);

    return encodeSegwitAddress(hash20, regtest);
}

String CapsCrypto::getAccountXpub(bool regtest) {
    if (!isUnlocked()) return "";
    char path[64];
    snprintf(path, sizeof(path), "m/84'/%d'/0'", CAPS_COINTYPE);
    HDPublicKey accountXpub = masterKey->derive(path).xpub();
    return accountXpub.xpub();
}

bool CapsCrypto::parsePSBTFile(const String& base64Content, UnsignedTxDetails& outDetails) {
    PSBT psbt;
    size_t parsed = psbt.parseBase64(base64Content);
    if (parsed == 0 || !psbt.isValid()) {
        outDetails.valid = false;
        return false;
    }

    outDetails.valid = true;
    outDetails.inputCount = psbt.tx.inputsNumber;
    outDetails.outputCount = psbt.tx.outputsNumber;
    outDetails.feeSats = psbt.fee();

    outDetails.recipient[0] = '\0';
    outDetails.amountSats = 0;

    for (uint8_t i = 0; i < psbt.tx.outputsNumber; i++) {
        bool isChange = false;
        if (isUnlocked()) {
            isChange = psbt.isMine(i, *masterKey);
        }

        if (!isChange || outDetails.amountSats == 0) {
            outDetails.amountSats = psbt.tx.txOuts[i].amount;
            char addr[80] = {0};
            psbt.tx.txOuts[i].address(addr, sizeof(addr), &Mainnet);
            
            // If uBitcoin encoded as bc1..., re-encode witness with btlcap
            if (strncmp(addr, "bc1", 3) == 0) {
                // Get witness program and re-encode
                uint8_t prog[40];
                size_t progLen = 0;
                int ver = 0;
                if (segwit_addr_decode(&ver, prog, &progLen, "bc", addr)) {
                    segwit_addr_encode(addr, CAPS_HRP, ver, prog, progLen);
                }
            }
            strncpy(outDetails.recipient, addr, sizeof(outDetails.recipient) - 1);
        }
    }

    return true;
}

bool CapsCrypto::signPSBT(const String& base64Content, String& signedBase64, String& errorMsg) {
    if (!isUnlocked()) {
        errorMsg = "Vault is locked!";
        return false;
    }

    PSBT psbt;
    size_t parsed = psbt.parseBase64(base64Content);
    if (parsed == 0 || !psbt.isValid()) {
        errorMsg = "Invalid PSBT payload";
        return false;
    }

    uint8_t signedCount = psbt.sign(*masterKey);
    if (signedCount == 0) {
        errorMsg = "No matching inputs found to sign";
        return false;
    }

    signedBase64 = psbt.toBase64();
    return true;
}

String CapsCrypto::formatSats(uint64_t sats) {
    double caps = (double)sats / 1e8;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4f BCAP", caps);
    return String(buf);
}

uint64_t CapsCrypto::capsToSats(double caps) {
    return (uint64_t)round(caps * 1e8);
}

double CapsCrypto::satsToCaps(uint64_t sats) {
    return (double)sats / 1e8;
}
