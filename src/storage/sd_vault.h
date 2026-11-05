#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "config.h"
#include "vault_types.h"

class SDVault {
public:
    SDVault();
    bool begin();
    bool isMounted() const;

    // PSBT file operations
    bool findUnsignedFile(String& filename, String& content);
    bool saveSignedPSBT(const String& baseFilename, const String& signedBase64);
    
    // File listing & inspection
    void listFiles();

private:
    bool mounted = false;
    SPIClass sdSpi;
};

extern SDVault VaultSD;
