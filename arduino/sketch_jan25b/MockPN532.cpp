// MockPN532.cpp
#include "MockPN532.h"

// Tag UID di esempio
const uint8_t MockPN532::MOCK_TAGS[2][7] = {
    {0x04, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F},
    {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}
};

MockPN532::MockPN532() : 
    lastReadTime(0),
    currentTagIndex(0),
    tagPresent(false) {}

bool MockPN532::begin() {
    return true;
}

bool MockPN532::readPassiveTargetID(uint8_t cardBaudRate, uint8_t* uid, uint8_t* uidLength){

    // Simula intervallo minimo tra letture
    if (millis() - lastReadTime < 100) {
        return false;
    }
    
    // Alterna tra i tag mock
    currentTagIndex = (currentTagIndex + 1) % NUM_MOCK_TAGS;
    memcpy(uid, MOCK_TAGS[currentTagIndex], 7);
    *uidLength = 7;
    
    lastReadTime = millis();
    tagPresent = true;
    return true;
}