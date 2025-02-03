#ifndef LIGHTWEIGHT_CRYPTO_H
#define LIGHTWEIGHT_CRYPTO_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

class LightweightCrypto {
private:
    uint8_t key[16];
    
    void xorBlock(uint8_t* data, const uint8_t* key, size_t len);
    uint32_t rotateLeft(uint32_t value, uint8_t bits);
    uint32_t rotateRight(uint32_t value, uint8_t bits);
    
    // Funzioni ausiliarie TEA per la cifratura di un blocco da 8 byte
    void TEA_encrypt_block(uint8_t* block, const uint8_t keyBytes[16]);
    void TEA_decrypt_block(uint8_t* block, const uint8_t keyBytes[16]);
    
public:
    LightweightCrypto();
    void setKey(const uint8_t* newKey, size_t keyLen);
    void encrypt(uint8_t* data, size_t len);
    void decrypt(uint8_t* data, size_t len);
    void hash(const uint8_t* data, size_t len, uint8_t* hashOut);
    void generateMAC(const uint8_t* data, size_t len, uint8_t* mac);
};

#endif
