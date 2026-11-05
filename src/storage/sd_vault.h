#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include "config.h"
#include "vault_types.h"

#define MAX_GALLERY_CARDS 32

class SDVault {
public:
    SDVault();
    bool begin();
    bool isMounted() const;

    // PSBT file operations
    bool findUnsignedFile(String& filename, String& content);
    bool saveSignedPSBT(const String& baseFilename, const String& signedBase64);
    
    // Wasteland Collectible Cards
    uint32_t scanCards(CollectibleCard* cardList, uint32_t maxCards);
    bool loadCardMetadata(const String& jsonPath, CollectibleCard& card);
    bool drawCardBMP(const char* filename, TFT_eSPI& tft, int16_t x, int16_t y, int16_t maxW = 140, int16_t maxH = 160);

    // File listing & inspection
    void listFiles();

private:
    bool mounted = false;
    SPIClass sdSpi;
};

extern SDVault VaultSD;
