# Documentazione di Sicurezza del Sistema

## 📋 Indice
1. [Overview del Sistema](#overview-del-sistema)
2. [Architettura di Sicurezza](#architettura-di-sicurezza)
3. [Dettagli Implementativi](#dettagli-implementativi)
4. [Test di Sicurezza](#test-di-sicurezza)
5. [Limitazioni e Miglioramenti](#limitazioni-e-miglioramenti)

## Overview del Sistema
Il sistema implementa un controllo accessi basato su tag NFC con verifica sia locale che remota. L'architettura si compone di:
- Arduino UNO R4 WiFi con lettore PN532
- Server MQTT con TLS
- Database SQLite per la gestione degli accessi

### Flusso di Autenticazione
1. Lettura tag NFC
2. Verifica locale nella cache cifrata
3. Se non presente, verifica remota via MQTT/TLS
4. Logging dell'evento
5. Feedback visivo su matrice LED

## Architettura di Sicurezza

### Livello Hardware
- **Lettore NFC PN532**
  - Rate limiting sulle letture
  - Protezione da replay attack
  - Buffer overflow protection

- **Arduino UNO R4 WiFi**
  - Secure storage in EEPROM cifrata
  - Protezione della memoria
  - LED matrix per feedback sicuro

### Livello Trasporto
- **TLS 1.2**
  - ECDHE-RSA per key exchange
  - AES128-GCM per cifratura
  - SHA256 per hashing
  - Perfect Forward Secrecy

### Livello Applicativo
- **Crittografia Leggera**
  - TEA per cifratura
  - SipHash modificato per MAC
  - IV casuali per ogni messaggio

- **Autenticazione**
  - Verifica a due livelli (locale/remota)
  - Cache sicura degli UID
  - Rate limiting sulle richieste

## Dettagli Implementativi

### Gestione Chiavi, Cifratura Tag, Messaggi sicuri
```cpp
// Esempio di gestione chiavi in SecureTagCache
SecureTagCache::SecureTagCache() : numTags(0) {
    uint8_t key[16] = {0x42}; // In produzione caricare da secure storage
    crypto.setKey(key, 16);
}

// Esempio di cifratura tag
bool SecureTagCache::addTag(const uint8_t* uid) {
    // Generazione IV sicura
    for (int i = 0; i < 8; i++) {
        cache[numTags].iv[i] = random(256);
    }
    // Cifratura e MAC
    crypto.encrypt(cache[numTags].data, 32);
    crypto.generateMAC(cache[numTags].data, 32, cache[numTags].mac);
}

// Esempio di messaggio sicuro
void NFCManager::sendSecureMessage(const char* topic, const uint8_t* data, size_t len) {
    String secureMessage = prepareSecureMessage(data, len);
    mqtt.beginMessage(topic);
    mqtt.print(secureMessage);
    mqtt.endMessage();
}
```