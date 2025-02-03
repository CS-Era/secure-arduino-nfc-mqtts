#include "LightweightCrypto.h"


/**
 * @brief Costruttore della classe LightweightCrypto
 * @details Inizializza la chiave interna a zero per sicurezza
 * @security Assicura che non ci siano valori residui nella chiave
 */
LightweightCrypto::LightweightCrypto() {
    memset(key, 0, sizeof(key));
}

/**
 * @brief Imposta la chiave di cifratura
 * @param newKey Puntatore alla nuova chiave
 * @param keyLen Lunghezza della nuova chiave
 * @security 
 *  - Pulisce la vecchia chiave prima di impostare la nuova
 *  - Limita la lunghezza della chiave al massimo consentito
 */
void LightweightCrypto::setKey(const uint8_t* newKey, size_t keyLen) {
    memset(key, 0, sizeof(key));
    memcpy(key, newKey, min(keyLen, sizeof(key)));
}

/**
 * @brief Esegue XOR tra dati e chiave
 * @param data Puntatore ai dati da processare
 * @param key Puntatore alla chiave
 * @param len Lunghezza dei dati
 * @details La chiave viene ripetuta ciclicamente se più corta dei dati
 */
void LightweightCrypto::xorBlock(uint8_t* data, const uint8_t* key, size_t len) {
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key[i % 16];
    }
}

/**
 * @brief Esegue rotazione a sinistra su un valore a 32 bit
 * @param value Valore da ruotare
 * @param bits Numero di posizioni per la rotazione
 * @return Il valore ruotato
 * @details Operazione fondamentale per molti algoritmi crittografici
 */
uint32_t LightweightCrypto::rotateLeft(uint32_t value, uint8_t bits) {
    return (value << bits) | (value >> (32 - bits));
}

/**
 * @brief Esegue rotazione a destra su un valore a 32 bit
 * @param value Valore da ruotare
 * @param bits Numero di posizioni per la rotazione
 * @return Il valore ruotato
 * @details Operazione fondamentale per molti algoritmi crittografici
 */
uint32_t LightweightCrypto::rotateRight(uint32_t value, uint8_t bits) {
    return (value >> bits) | (value << (32 - bits));
}

/**
 * @brief Cifra un blocco di dati usando l'algoritmo TEA
 * @param block Blocco di 8 byte da cifrare
 * @param keyBytes Chiave di cifratura di 16 byte
 * @security Implementa 32 round di cifratura TEA
 */
void LightweightCrypto::TEA_encrypt_block(uint8_t* block, const uint8_t keyBytes[16]) {
    uint32_t v0, v1;
    memcpy(&v0, block, 4);
    memcpy(&v1, block + 4, 4);
    
    uint32_t keyParts[4];
    memcpy(keyParts, keyBytes, 16);
    
    uint32_t sum = 0;
    uint32_t delta = 0x9E3779B9;
    for (int i = 0; i < 32; i++) {
        sum += delta;
        v0 += ((v1 << 4) + keyParts[0]) ^ (v1 + sum) ^ ((v1 >> 5) + keyParts[1]);
        v1 += ((v0 << 4) + keyParts[2]) ^ (v0 + sum) ^ ((v0 >> 5) + keyParts[3]);
    }
    
    memcpy(block, &v0, 4);
    memcpy(block + 4, &v1, 4);
}

/**
 * @brief Decifra un blocco di dati usando l'algoritmo TEA
 * @param block Blocco di 8 byte da decifrare
 * @param keyBytes Chiave di cifratura di 16 byte
 * @security Implementa 32 round di decifratura TEA
 */
void LightweightCrypto::TEA_decrypt_block(uint8_t* block, const uint8_t keyBytes[16]) {
    uint32_t v0, v1;
    memcpy(&v0, block, 4);
    memcpy(&v1, block + 4, 4);
    
    uint32_t keyParts[4];
    memcpy(keyParts, keyBytes, 16);
    
    uint32_t delta = 0x9E3779B9;
    uint32_t sum = delta * 32;
    for (int i = 0; i < 32; i++) {
        v1 -= ((v0 << 4) + keyParts[2]) ^ (v0 + sum) ^ ((v0 >> 5) + keyParts[3]);
        v0 -= ((v1 << 4) + keyParts[0]) ^ (v1 + sum) ^ ((v1 >> 5) + keyParts[1]);
        sum -= delta;
    }
    
    memcpy(block, &v0, 4);
    memcpy(block + 4, &v1, 4);
}

/**
 * @brief Cifra dati utilizzando l'algoritmo TEA in modalità ECB
 * @param data Puntatore ai dati da cifrare
 * @param len Lunghezza dei dati in bytes (deve essere multiplo di 8)
 * @security ATTENZIONE: La modalità ECB è considerata insicura per blocchi correlati
 * @pre len deve essere multiplo di 8 bytes
 * @details Processa i dati in blocchi da 8 bytes utilizzando TEA block cipher
 */
void LightweightCrypto::encrypt(uint8_t* data, size_t len) {
    size_t numBlocks = len / 8;
    for (size_t i = 0; i < numBlocks; i++) {
        TEA_encrypt_block(data + i * 8, key);
    }
}

/**
 * @brief Decifra dati utilizzando l'algoritmo TEA in modalità ECB
 * @param data Puntatore ai dati da decifrare
 * @param len Lunghezza dei dati in bytes (deve essere multiplo di 8)
 * @security ATTENZIONE: La modalità ECB è considerata insicura per blocchi correlati
 * @pre len deve essere multiplo di 8 bytes
 * @details Processa i dati in blocchi da 8 bytes utilizzando TEA block cipher
 */
void LightweightCrypto::decrypt(uint8_t* data, size_t len) {
    size_t numBlocks = len / 8;
    for (size_t i = 0; i < numBlocks; i++) {
        TEA_decrypt_block(data + i * 8, key);
    }
}

/**
 * @brief Calcola hash utilizzando una versione semplificata di SipHash
 * @param data Puntatore ai dati di input
 * @param len Lunghezza dei dati di input
 * @param hashOut Buffer di output per l'hash (deve essere di almeno 8 bytes)
 * @details Implementa una versione semplificata di SipHash con costanti predefinite
 * @security Fornisce 64 bit di output, adatto per MAC ma non per uso crittografico generale
 */
void LightweightCrypto::hash(const uint8_t* data, size_t len, uint8_t* hashOut) {
    uint32_t v0 = 0x736f6d65;
    uint32_t v1 = 0x646f7261;
    uint32_t v2 = 0x6c796765;
    uint32_t v3 = 0x74656462;
    
    for (size_t i = 0; i < len; i++) {
        v3 ^= data[i];
        v0 += v1;
        v1 = rotateLeft(v1, 13);
        v1 ^= v0;
        v0 = rotateLeft(v0, 32);
        v2 += v3;
        v3 = rotateLeft(v3, 16);
        v3 ^= v2;
        v0 += v3;
        v3 = rotateLeft(v3, 21);
        v3 ^= v0;
        v2 += v1;
        v1 = rotateLeft(v1, 17);
        v1 ^= v2;
        v2 = rotateLeft(v2, 32);
    }
    
    memcpy(hashOut, &v0, 4);
    memcpy(hashOut + 4, &v1, 4);
}

/**
 * @brief Genera un MAC (Message Authentication Code)
 * @param data Dati su cui generare il MAC
 * @param len Lunghezza dei dati
 * @param mac Buffer di output per il MAC (8 byte)
 * @security Implementa MAC = hash(key || data) per garantire integrità
 */
void LightweightCrypto::generateMAC(const uint8_t* data, size_t len, uint8_t* mac) {
    // Alloca il buffer per concatenare chiave e dati
    uint8_t* buffer = new uint8_t[16 + len];
    
    // Concatena key || data
    memcpy(buffer, key, 16);
    memcpy(buffer + 16, data, len);
    
    // Genera il MAC
    hash(buffer, 16 + len, mac);
    
    // Pulisci e dealloca il buffer per sicurezza
    memset(buffer, 0, 16 + len);
    delete[] buffer;
}
