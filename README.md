# 🔒 IoT Security Project - NFC Access Control System

Sistema di controllo accessi sicuro basato su **Arduino UNO R4 WiFi** con lettore **NFC PN532** e **server MQTT con TLS**. Il progetto implementa un sistema di autenticazione a due livelli che combina verifica locale e remota dei tag NFC con comunicazioni cifrate e sicure.

---
## 📌 Prerequisiti
🔹 **Hardware**  
- 🛠️ Arduino UNO R4 WiFi  
- 📡 Modulo NFC PN532
- 🔌 Cavi jumper per collegamenti
- 🔌 Cavo USB-C  

🔹 **Software**  
- 🖥️ Node.js e npm  
- ⚙️ Arduino IDE  

---
## 📦 Librerie Arduino Necessarie
Installa queste librerie dall'**Arduino Library Manager**:
- 📡 **ArduinoMqttClient** by Arduino  
- 🔄 **Adafruit BusIO** by Adafruit  
- 🏷️ **Adafruit PN532** by Adafruit 
- 📑 **ArduinoJson** by Benoît Blanchnon  
- 🎯 **ArduinoGraphics** by Arduino (per la matrice LED)
- 🔆 **Arduino_LED_Matrix** by Arduino (per il feedback visivo)

Inserisci le librerie presenti in `arduino/pn532_libraries` nella cartella libraries di Arduino

---

## 🚀 Setup del Progetto

### 1️⃣ Clona il repository
```bash
git clone https://github.com/CS-Era/iot-security-project.git
cd iot-security-project
```

### 2️⃣ Installa le dipendenze del server
```bash
cd server
npm install
```
### 2️⃣ Installa le dipendenze del tool di setup
```bash
cd ../tools
npm install
```
### 3️⃣ Avvia il setup
```bash
node setup_new_arduino.js
```
### 4️⃣ Segui le indicazioni a schermo

### 5️⃣ Avvia il server MQTT
```bash
node server/server.js
```
   

## ⚙️ Funzionalità dello Script di Setup

Il setup automatizzato esegue diverse operazioni per configurare Arduino UNO R4 WiFi e il server MQTT in pochi passaggi  

### 🔍 1️⃣ Rilevamento e Identificazione  
✔️ Riconoscimento automatico della scheda Arduino collegata  

### 🔑 2️⃣ Generazione Credenziali Sicure  
✔️ Creazione di un'API key unica e sicura per l'autenticazione del dispositivo  

### 📶 3️⃣ Configurazione della Rete  
✔️ Richiesta interattiva delle credenziali WiFi:  
   - 📡 SSID (nome della rete WiFi)  
   - 🔑 Password WiFi (per la connessione)  
✔️ Richiesta dell'IP del Broker MQTT per la comunicazione  

### 🛠️ 4️⃣ Setup dei File
✔️ Configurazione automatica dei file di sistema .env e config.h  
  

🎯 **Risultato:**  
⚡ Arduino è pronto per comunicare in modo sicuro con il broker MQTT utilizzando TLS e autenticazione con API key 🚀  

## 🛡️ Funzionalità di Sicurezza

### 1️⃣ Autenticazione Multi-livello
- **Verifica Locale**
 - Cache sicura degli UID autorizzati
 - Cifratura dei tag memorizzati
 - MAC per integrità dei dati

- **Verifica Remota**
 - Server MQTT con autenticazione dei dispositivi
 - Verifica degli UID nel database
 - Logging degli accessi

### 2️⃣ Sicurezza delle Comunicazioni
- **Transport Layer Security (TLS 1.2)**
 - ECDHE per key exchange
 - RSA per autenticazione
 - AES128-GCM per cifratura
 - SHA256 per integrità

- **Protezione a Livello Applicativo**
 - Cifratura TEA con chiave 128 bit
 - MAC basato su SipHash modificato
 - IV casuali per protezione replay
 - Padding sicuro dei messaggi

### 3️⃣ Sistema di Logging
- Registrazione eventi di:
 - Autenticazione dispositivi
 - Verifica tag NFC
 - Accessi consentiti/negati
 - Errori di sistema
- Timestamp per ogni evento
- Hash dei dati sensibili nei log

### 4️⃣ Feedback e Monitoraggio
- Display LED integrato per:
 - Stato accessi
 - Errori di sistema
 - Stato connessione
- Monitoraggio real-time via MQTT
- Log dettagliati su Serial Monitor

## ⚙️ Implementazione Tecnica

### 1️⃣ Sicurezza Hardware
- Utilizzo sicuro della memoria EEPROM
- Rate limiting sulle letture NFC
- Gestione sicura delle chiavi crittografiche

### 2️⃣ Crittografia Leggera
- **TEA (Tiny Encryption Algorithm)**
 - Modalità ECB per UID (7 byte)
 - Chiave simmetrica 128 bit
 - Implementazione ottimizzata per Arduino

- **MAC Personalizzato**
 - Basato su SipHash
 - Input: key || IV || ciphertext
 - Output: 64 bit
 - IV random a 8 byte

### 3️⃣ Struttura Database
- Tabella UID cifrati
- Tabella log eventi
- Backup automatico
- Pulizia periodica

### 4️⃣ Note Implementative
- ECB utilizzato solo per UID (7 byte)
- Rate limiting su tutte le operazioni critiche
- Gestione sicura della memoria
- Sanitizzazione degli input

Per ulteriori dettagli implementativi e di sicurezza, consultare la documentazione nel codice sorgente.
