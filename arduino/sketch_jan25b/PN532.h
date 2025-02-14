#ifndef PN532_H
#define PN532_H

#include <Arduino.h>  // Per uint8_t

class PN532_SPI;  // Forward declaration
class PN532;      // Forward declaration

class NFCReader {
private:
    PN532_SPI* pn532spi;
    PN532* nfc;
    static const uint8_t PN532_SS = 10;
    
public:
    NFCReader();
    ~NFCReader();
    bool begin();
    bool readPassiveTargetID(uint8_t cardBaudRate, uint8_t* uid, uint8_t* uidLength);
};

#endif