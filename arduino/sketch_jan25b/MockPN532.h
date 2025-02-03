// MockPN532.h
#ifndef MOCK_PN532_H
#define MOCK_PN532_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

class MockPN532 {
private:
    static const uint8_t NUM_MOCK_TAGS = 2;
    static const uint8_t MOCK_TAGS[2][7];
    uint32_t lastReadTime;
    uint8_t currentTagIndex;
    bool tagPresent;
    
public:
    MockPN532();
    bool begin();
    bool readPassiveTargetID(uint8_t cardBaudRate, uint8_t* uid, uint8_t* uidLength);
};

#endif