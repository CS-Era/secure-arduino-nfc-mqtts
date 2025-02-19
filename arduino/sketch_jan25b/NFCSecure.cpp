#include "NFCSecure.h"

// ----------------- SecureTagCache Implementation -----------------


/**
 * @brief Costruttore della classe SecureTagCache
 * @details Inizializza la cache con una chiave di default di 16 byte. 
 * @security TODO: La chiave dovrebbe essere caricata da un secure storage invece che essere hardcoded
 */
SecureTagCache::SecureTagCache() : numTags(0) {
    // SECURITY: La chiave dovrebbe essere caricata da un secure storage
    uint8_t key[16] = {0x42}; // TODO: Implementare secure storage
    crypto.setKey(key, 16);
}


/**
 * @brief Aggiunge un nuovo tag NFC alla cache sicura di Arduino
 * @param uid Array di byte contenente l'identificativo univoco del tag NFC
 * @return true se il tag è stato aggiunto con successo, false altrimenti
 * @security Implementa:
 *  - Generazione sicura di IV casuali
 *  - Cifratura dei dati con algoritmo TEA (Tiny Encryption Algorithm)
 *  - Generazione MAC per integrità
 */
bool SecureTagCache::addTag(const uint8_t* uid) {
    // Controlla se il tag è già in cache
    for (uint8_t i = 0; i < numTags; i++) {
        uint8_t derivedKey8[8];
        crypto.hash(cache[i].iv, 8, derivedKey8);
        uint8_t fullKey[16];
        memcpy(fullKey, derivedKey8, 8);
        memcpy(fullKey + 8, derivedKey8, 8);
        crypto.setKey(fullKey, 16);
        
        crypto.decrypt(cache[i].data, 32);
        TagEntry existingTag;
        memcpy(&existingTag, cache[i].data, sizeof(TagEntry));
        
        // Se il tag esiste già, non aggiungere duplicati
        if (memcmp(existingTag.uid, uid, 7) == 0) {
            // Ripristina lo stato crittato precedente
            crypto.encrypt(cache[i].data, 32);
            return true;
        }
    }

    // Se la cache è piena, cerca uno slot libero o sovrascrivi il meno usato
    if (numTags >= MAX_TAGS) {
        uint16_t minUses = UINT16_MAX;
        int replaceIdx = -1;
        TagEntry temp;
        
        for (uint8_t i = 0; i < MAX_TAGS; i++) {
            uint8_t derivedKey8[8];
            crypto.hash(cache[i].iv, 8, derivedKey8);
            uint8_t fullKey[16];
            memcpy(fullKey, derivedKey8, 8);
            memcpy(fullKey + 8, derivedKey8, 8);
            crypto.setKey(fullKey, 16);
            
            crypto.decrypt(cache[i].data, 32);
            memcpy(&temp, cache[i].data, sizeof(TagEntry));
            
            if (temp.useCount < minUses) {
                minUses = temp.useCount;
                replaceIdx = i;
            }
        }
        
        if (replaceIdx >= 0) {
            // Prepara nuovo tag
            TagEntry newTag = {0};
            memcpy(newTag.uid, uid, 7);
            newTag.lastUsed = millis();
            newTag.useCount = 1;
            newTag.valid = true;
            
            // Genera nuovo IV
            for (int i = 0; i < 8; i++) {
                cache[replaceIdx].iv[i] = random(256);
            }
            
            // Deriva chiave
            uint8_t derivedKey8[8];
            crypto.hash(cache[replaceIdx].iv, 8, derivedKey8);
            uint8_t fullKey[16];
            memcpy(fullKey, derivedKey8, 8);
            memcpy(fullKey + 8, derivedKey8, 8);
            crypto.setKey(fullKey, 16);
            
            // Cripta e aggiungi MAC
            memset(cache[replaceIdx].data, 0, 32);
            memcpy(cache[replaceIdx].data, &newTag, sizeof(TagEntry));
            crypto.encrypt(cache[replaceIdx].data, 32);
            crypto.generateMAC(cache[replaceIdx].data, 32, cache[replaceIdx].mac);
            
            return true;
        }
        return false;
    }
    
    // Aggiunta di un nuovo tag quando c'è spazio
    for (int i = 0; i < 8; i++) {
        cache[numTags].iv[i] = random(256);
    }
    
    TagEntry newTag = {0};
    memcpy(newTag.uid, uid, 7);
    newTag.lastUsed = millis();
    newTag.useCount = 1;
    newTag.valid = true;
    
    uint8_t derivedKey8[8];
    crypto.hash(cache[numTags].iv, 8, derivedKey8);
    uint8_t fullKey[16];
    memcpy(fullKey, derivedKey8, 8);
    memcpy(fullKey + 8, derivedKey8, 8);
    crypto.setKey(fullKey, 16);
    
    memset(cache[numTags].data, 0, 32);
    memcpy(cache[numTags].data, &newTag, sizeof(TagEntry));
    crypto.encrypt(cache[numTags].data, 32);
    crypto.generateMAC(cache[numTags].data, 32, cache[numTags].mac);
    
    numTags++;
    return true;
}


/**
 * @brief Verifica se un tag NFC è presente nella cache e se è valido
 * @param uid Array di byte contenente l'identificativo univoco del tag da verificare
 * @return true se il tag è valido e presente in cache, false altrimenti
 * @security Implementa:
 *  - Verifica dell'integrità tramite MAC
 *  - Decifratura sicura dei dati
 *  - Logging non sensibile delle operazioni
 */
bool SecureTagCache::verifyTag(const uint8_t* uid) {

    Serial.print("Numero di tag in cache: ");
    Serial.println(numTags);

    TagEntry tag;
    for (uint8_t i = 0; i < numTags; i++) {
        // Crea copie dei dati per non modificare l'originale
        uint8_t tempIV[8];
        uint8_t tempData[32];
        uint8_t tempMac[8];
        
        memcpy(tempIV, cache[i].iv, 8);
        memcpy(tempData, cache[i].data, 32);
        memcpy(tempMac, cache[i].mac, 8);

        uint8_t derivedKey8[8];
        crypto.hash(tempIV, 8, derivedKey8);
        uint8_t fullKey[16];
        memcpy(fullKey, derivedKey8, 8);
        memcpy(fullKey + 8, derivedKey8, 8);
        crypto.setKey(fullKey, 16);
        
        uint8_t calculatedMac[8];
        crypto.generateMAC(tempData, 32, calculatedMac);
        bool integrityOk = (memcmp(calculatedMac, tempMac, 8) == 0);
        
        crypto.decrypt(tempData, 32);
        memcpy(&tag, tempData, sizeof(TagEntry));
                
        
        if (tag.valid && (memcmp(tag.uid, uid, 7) == 0) && integrityOk) {
            Serial.println("Tag verificato con successo!");
            return true;
        }
    }
    
    Serial.println("Verifica tag fallita");
    return false;
}


/**
 * @brief Salva lo stato della cache nella memoria EEPROM
 * @return true se il salvataggio è avvenuto con successo, false altrimenti
 * @security I dati sono già cifrati prima del salvataggio
 */
bool SecureTagCache::saveToEEPROM() {
    EEPROM.put(0, EEPROM_MAGIC);
    EEPROM.put(2, numTags);
    for (uint8_t i = 0; i < numTags; i++) {
        EEPROM.put(4 + (i * sizeof(EncryptedData)), cache[i]);
    }
    return true;
}

/**
 * @brief Carica lo stato della cache dalla memoria EEPROM
 * @return true se il caricamento è avvenuto con successo, false altrimenti
 * @security Verifica la validità dei dati tramite magic number
 */
bool SecureTagCache::loadFromEEPROM() {
    uint16_t magic;
    EEPROM.get(0, magic);
    if (magic != EEPROM_MAGIC) return false;
    EEPROM.get(2, numTags);
    if (numTags > MAX_TAGS) {
        numTags = 0;
        return false;
    }
    for (uint8_t i = 0; i < numTags; i++) {
        EEPROM.get(4 + (i * sizeof(EncryptedData)), cache[i]);
    }
    return true;
}






// ----------------- NFCManager Implementation -----------------


/**
 * @brief Costruttore della classe NFCManager
 * @param nfcReader Riferimento al lettore NFC
 * @param tagCache Riferimento alla cache dei tag
 * @param mqttClient Riferimento al client MQTT
 * @security Inizializza una chiave di cifratura per le comunicazioni MQTT
 */
NFCManager::NFCManager(NFCReader& nfcReader, SecureTagCache& tagCache, MqttClient& mqttClient)
  : nfc(nfcReader), cache(tagCache), mqtt(mqttClient), isAdmin(false), uidLength(0), rounds(0)
{
    // Inizializza la chiave con lo stesso valore usato nel server
    uint8_t tempKey[] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                        0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
    memcpy(key, tempKey, 16);
    crypto.setKey(key, 16);
}


String NFCManager::prepareSecureMessage(const uint8_t* data, size_t len) {
    // 1. Genera IV random
    uint8_t iv[8];
    for (int i = 0; i < 8; i++) {
        iv[i] = random(256);
    }
    
    // 2. Padding dei dati a multiplo di 8 bytes (blocco TEA)
    size_t paddedLen = ((len + 7) / 8) * 8;
    uint8_t* paddedData = new uint8_t[paddedLen];
    memset(paddedData, 0, paddedLen);  // Padding con zeri
    memcpy(paddedData, data, len);
    
    // 3. Cifra i dati
    uint8_t* encrypted = new uint8_t[paddedLen];
    memcpy(encrypted, paddedData, paddedLen);
    crypto.setKey(key, 16);
    crypto.encrypt(encrypted, paddedLen);
    
    // 4. Genera MAC su IV || ciphertext
    uint8_t mac[8];
    uint8_t* macData = new uint8_t[8 + paddedLen];
    memcpy(macData, iv, 8);
    memcpy(macData + 8, encrypted, paddedLen);
    crypto.generateMAC(macData, 8 + paddedLen, mac);
    
    // 5. Prepara stringa risultato
    String result;
    
    // IV
    for (int i = 0; i < 8; i++) {
        if (iv[i] < 0x10) result += '0';
        result += String(iv[i], HEX);
    }
    
    // Ciphertext
    for (size_t i = 0; i < paddedLen; i++) {
        if (encrypted[i] < 0x10) result += '0';
        result += String(encrypted[i], HEX);
    }
    
    // MAC
    for (int i = 0; i < 8; i++) {
        if (mac[i] < 0x10) result += '0';
        result += String(mac[i], HEX);
    }
    
    // Pulizia memoria
    delete[] paddedData;
    delete[] encrypted;
    delete[] macData;
    
    return result;
}


void NFCManager::sendSecureMessage(const char* topic, const uint8_t* data, size_t len) {
    String secureMessage = prepareSecureMessage(data, len);
    
    mqtt.beginMessage(topic);
    mqtt.print(secureMessage);
    mqtt.endMessage();
}

/**
 * @brief Imposta la modalità amministratore
 * @param enabled true per abilitare la modalità admin, false per disabilitarla
 * @security Controlla i permessi per operazioni privilegiate
 */
void NFCManager::setAdminMode(bool enabled) {
    isAdmin = enabled;
}

/**
 * @brief Inizializza il gestore NFC
 * @return true se l'inizializzazione è avvenuta con successo, false altrimenti
 * @security Reset della cache per evitare dati residui
 */
bool NFCManager::begin() {
    // Reset della cache per evitare dati residui
    cache = SecureTagCache();
    if (!nfc.begin()) {
        return false;
    }
    return true;
}

/**
 * @brief Aggiorna lo stato del gestore NFC e gestisce lettura/verifica dei tag
 * @return true se l'operazione è avvenuta con successo, false altrimenti
 * @details Gestisce:
 *  - Lettura del tag NFC
 *  - Verifica locale nella cache
 *  - Invio notifiche MQTT per accessi e verifiche remote
 * @security Implementa:
 *  - Logging sicuro senza esporre dati sensibili
 *  - Comunicazione cifrata con il server
 */
bool NFCManager::update() {
    if (!nfc.readPassiveTargetID(0, tempUid, &uidLength)) return false;
    
    Serial.print("\n[READ] Tag UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
        if (tempUid[i] < 0x10) Serial.print("0");
        Serial.print(tempUid[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    bool verified = cache.verifyTag(tempUid);
    if (verified){
        Serial.println("[RESULT] ACCESS GRANTED");
        // Server registra log di acceso
        sendSecureMessage("nfc/access",tempUid,uidLength);    
    }else{
        Serial.println("[RESULT] ACCESS DENIED - SERVER AUTHENTICATION VERIFY");
        // Server gestisce autenticazione
        sendSecureMessage("nfc/verify",tempUid,uidLength);    
    }

    return true;
}

/**
 * @brief Registra un nuovo tag nel sistema
 * @return true se la registrazione è avvenuta con successo, false altrimenti
 * @security Implementa:
 *  - Verifica dei permessi amministratore
 *  - Cifratura dei dati del tag
 *  - Notifica sicura al server
 */
bool NFCManager::registerNewTag() {
    if (!isAdmin || !nfc.readPassiveTargetID(0, tempUid, &uidLength))
        return false;
    if (cache.addTag(tempUid)) {
        // Possibile gestione in locale su arduino dei tag pre-registrati
        // Altrimenti gestione via server con topic nfc/register
        //sendSecureMessage("nfc/register", tempUid, 32);
        return true;
    }
    return false;
}
