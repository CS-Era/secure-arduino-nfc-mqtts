#include "PN532.h"
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>

NFCReader::NFCReader() {
    pn532spi = new PN532_SPI(SPI, PN532_SS);
    nfc = new PN532(*pn532spi);
}

NFCReader::~NFCReader() {
    delete nfc;
    delete pn532spi;
}

bool NFCReader::begin() {
    nfc->begin();
    uint32_t versiondata = nfc->getFirmwareVersion();
    if (!versiondata) {
        return false;
    }
    nfc->SAMConfig();
    return true;
}

bool NFCReader::readPassiveTargetID(uint8_t cardBaudRate, uint8_t* uid, uint8_t* uidLength) {
    return nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength);
}