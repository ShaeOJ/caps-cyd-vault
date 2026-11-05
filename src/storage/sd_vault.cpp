#include "sd_vault.h"

SDVault VaultSD;

SDVault::SDVault() : sdSpi(VSPI) {
    mounted = false;
}

bool SDVault::begin() {
    sdSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, sdSpi, 25000000)) {
        Serial.println("[SD] Mount failed or no card inserted");
        mounted = false;
        return false;
    }
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] No SD card attached");
        mounted = false;
        return false;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[SD] Mounted successfully: %llu MB\n", cardSize);
    mounted = true;
    return true;
}

bool SDVault::isMounted() const {
    return mounted;
}

bool SDVault::findUnsignedFile(String& filename, String& content) {
    if (!mounted && !begin()) {
        return false;
    }

    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
        return false;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = String(file.name());
            String lower = name;
            lower.toLowerCase();

            // Look for .psbt or .txt or .json containing PSBT, but ignore already signed files
            if ((lower.endsWith(".psbt") || lower.endsWith(".txn") || lower.endsWith(".txt")) &&
                !lower.endsWith("_signed.psbt") &&
                !lower.endsWith(".signed.txn") &&
                !lower.startsWith("signed_")) {
                
                size_t sz = file.size();
                if (sz > 0 && sz < 65536) {
                    content = file.readString();
                    content.trim();
                    filename = name;
                    file.close();
                    root.close();
                    return true;
                }
            }
        }
        file = root.openNextFile();
    }
    root.close();
    return false;
}

bool SDVault::saveSignedPSBT(const String& baseFilename, const String& signedBase64) {
    if (!mounted && !begin()) {
        return false;
    }

    String outName = "/signed_" + baseFilename;
    if (outName.startsWith("//")) outName = outName.substring(1);
    if (!outName.endsWith(".psbt")) outName += ".psbt";

    File f = SD.open(outName, FILE_WRITE);
    if (!f) {
        Serial.printf("[SD] Failed to open %s for writing\n", outName.c_str());
        return false;
    }

    size_t written = f.print(signedBase64);
    f.flush();
    f.close();

    Serial.printf("[SD] Wrote %u bytes to %s\n", written, outName.c_str());
    return written > 0;
}

void SDVault::listFiles() {
    if (!mounted && !begin()) return;
    File root = SD.open("/");
    File file = root.openNextFile();
    Serial.println("[SD] Files on card:");
    while (file) {
        Serial.printf("  %s (%u bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
}
