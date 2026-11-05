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

bool SDVault::loadCardMetadata(const String& jsonPath, CollectibleCard& card) {
    File f = SD.open(jsonPath, FILE_READ);
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) return false;

    card.serial = doc["serial"] | 0;
    card.tier = doc["tier"] | 1;
    const char* t = doc["title"] | "Wasteland Bottlecap";
    strncpy(card.title, t, sizeof(card.title) - 1);
    const char* r = doc["recipe_hash"] | "";
    strncpy(card.recipeHash, r, sizeof(card.recipeHash) - 1);

    return true;
}

uint32_t SDVault::scanCards(CollectibleCard* cardList, uint32_t maxCards) {
    if (!mounted && !begin()) return 0;

    // Ensure /cards directory exists
    if (!SD.exists("/cards")) {
        SD.mkdir("/cards");
        return 0;
    }

    File dir = SD.open("/cards");
    if (!dir || !dir.isDirectory()) return 0;

    uint32_t count = 0;
    File file = dir.openNextFile();
    while (file && count < maxCards) {
        if (!file.isDirectory()) {
            String name = String(file.name());
            String lower = name;
            lower.toLowerCase();

            if (lower.endsWith(".bmp")) {
                CollectibleCard& c = cardList[count];
                memset(&c, 0, sizeof(CollectibleCard));
                c.tier = 1;
                c.serial = count + 1;
                snprintf(c.title, sizeof(c.title), "Card #%u", count + 1);

                String fullBmp = name.startsWith("/") ? name : ("/cards/" + name);
                strncpy(c.imagePath, fullBmp.c_str(), sizeof(c.imagePath) - 1);

                // Look for matching .json
                String base = fullBmp.substring(0, fullBmp.length() - 4);
                String jsonPath = base + ".json";
                if (SD.exists(jsonPath)) {
                    strncpy(c.jsonPath, jsonPath.c_str(), sizeof(c.jsonPath) - 1);
                    loadCardMetadata(jsonPath, c);
                }
                count++;
            }
        }
        file = dir.openNextFile();
    }
    dir.close();
    Serial.printf("[SD] Scanned %u collectible cards from /cards/\n", count);
    return count;
}

bool SDVault::drawCardBMP(const char* filename, TFT_eSPI& tft, int16_t x, int16_t y, int16_t maxW, int16_t maxH) {
    if (!mounted && !begin()) return false;

    File bmpFile = SD.open(filename, FILE_READ);
    if (!bmpFile) {
        Serial.printf("[SD] BMP not found: %s\n", filename);
        return false;
    }

    // Verify 'BM' signature
    uint8_t sig[2];
    bmpFile.read(sig, 2);
    if (sig[0] != 'B' || sig[1] != 'M') {
        bmpFile.close();
        return false;
    }

    // Seek to pixel data offset
    bmpFile.seek(10);
    uint32_t bmpImageoffset = 0;
    bmpFile.read((uint8_t*)&bmpImageoffset, 4);

    // Seek to DIB width and height
    bmpFile.seek(18);
    int32_t bmpWidth = 0, bmpHeight = 0;
    bmpFile.read((uint8_t*)&bmpWidth, 4);
    bmpFile.read((uint8_t*)&bmpHeight, 4);

    uint16_t planes = 0, depth = 0;
    bmpFile.read((uint8_t*)&planes, 2);
    bmpFile.read((uint8_t*)&depth, 2);

    if (planes != 1 || (depth != 24 && depth != 16)) {
        bmpFile.close();
        return false;
    }

    bool flip = true;
    if (bmpHeight < 0) {
        bmpHeight = -bmpHeight;
        flip = false;
    }

    uint32_t rowSize = (bmpWidth * (depth / 8) + 3) & ~3;
    int16_t drawW = bmpWidth > maxW ? maxW : bmpWidth;
    int16_t drawH = bmpHeight > maxH ? maxH : bmpHeight;

    uint16_t lineBuffer[160];
    uint8_t rawRow[160 * 3 + 4];

    for (int row = 0; row < drawH; row++) {
        uint32_t pos = flip ? (bmpImageoffset + (bmpHeight - 1 - row) * rowSize) : (bmpImageoffset + row * rowSize);
        bmpFile.seek(pos);
        bmpFile.read(rawRow, rowSize);

        for (int col = 0; col < drawW; col++) {
            uint16_t color565;
            if (depth == 24) {
                uint8_t b = rawRow[col * 3];
                uint8_t g = rawRow[col * 3 + 1];
                uint8_t r = rawRow[col * 3 + 2];
                color565 = tft.color565(r, g, b);
            } else {
                color565 = rawRow[col * 2] | (rawRow[col * 2 + 1] << 8);
            }
            lineBuffer[col] = color565;
        }
        tft.pushImage(x, y + row, drawW, 1, lineBuffer);
    }

    bmpFile.close();
    return true;
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
