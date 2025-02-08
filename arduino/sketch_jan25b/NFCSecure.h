#ifndef NFC_SECURE_H
#define NFC_SECURE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoMqttClient.h>
#include "LightweightCrypto.h"
#include "MockPN532.h"

// SECURITY: Strutture dati con packed attribute per minimizzare memoria
struct TagEntry {
    uint8_t uid[7];        // UID del tag NFC
    uint32_t lastUsed;     // Timestamp ultimo utilizzo
    uint16_t useCount;     // Contatore utilizzi
    bool valid;            // Flag validità
} __attribute__((packed));

struct EncryptedData {
    uint8_t iv[8];         // Vettore di inizializzazione
    uint8_t data[32];      // Dati cifrati
    uint8_t mac[8];        // MAC per integrità
} __attribute__((packed));

class SecureTagCache {
private:
    static constexpr uint16_t EEPROM_MAGIC = 0xABCD;
    static const uint8_t MAX_TAGS = 5;
    LightweightCrypto crypto;
    EncryptedData cache[MAX_TAGS];
    uint8_t numTags;

public:
    SecureTagCache();
    bool addTag(const uint8_t* uid);
    bool verifyTag(const uint8_t* uid);
    bool saveToEEPROM();
    bool loadFromEEPROM();
};

class NFCManager {
private:
    MockPN532& nfc;
    SecureTagCache& cache;
    MqttClient& mqtt;
    LightweightCrypto crypto;
    bool isAdmin;
    uint8_t tempUid[7];
    uint8_t uidLength;
    uint8_t rounds;
    uint8_t key[16]; 



public:
    NFCManager(MockPN532& nfcReader, SecureTagCache& tagCache, MqttClient& mqttClient);
    void setAdminMode(bool enabled);
    bool update();
    bool begin();
    bool registerNewTag();

    
    void sendSecureMessage(const char* topic, const uint8_t* data, size_t len);
    String prepareSecureMessage(const uint8_t* data, size_t len);

};

#endif
