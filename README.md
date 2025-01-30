# 🔒 IoT Security Project - MQTT over TLS

Sistema di comunicazione sicura tra **Arduino UNO R4 WiFi** e un **server MQTT con TLS** e autenticazione dei dispositivi.  
Il progetto garantisce **trasmissioni cifrate e autenticazione sicura** tra i dispositivi IoT e il broker MQTT.

---

## 📌 Prerequisiti

🔹 **Hardware**  
- 🛠️ Arduino UNO R4 WiFi  
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
- 📢 **PubSubClient** by Nick O'Leary  

---

## 🚀 Setup del Progetto

### 1️⃣ Clona il repository
```bash
git clone https://github.com/CS-Era/iot-security-project.git
cd iot-security-project
```

### 2️⃣ Installa le dipendenze
```bash
npm install
```
   
### 3️⃣ Collega Arduino UNO R4 WiFi via USB-C

### 4️⃣ Configura Arduino automaticamente
```bash
node tools/setup_new_arduino.js
```

### 5️⃣ Avvia il server MQTT
```bash
node server/server.js
```
   

## ⚙️ Funzionalità dello Script di Setup

🔧 **Il setup automatizzato esegue diverse operazioni per configurare Arduino UNO R4 WiFi e il server MQTT in pochi passaggi.**  

### 🔍 1️⃣ Rilevamento e Identificazione  
✔️ Riconoscimento automatico della scheda Arduino collegata  
✔️ Identificazione univoca del MAC Address del dispositivo  

### 🔑 2️⃣ Generazione Credenziali Sicure  
✔️ Creazione di un'API key unica e sicura per l'autenticazione del dispositivo  

### 📶 3️⃣ Configurazione della Rete  
✔️ Richiesta interattiva delle credenziali WiFi:  
   - 📡 SSID (nome della rete WiFi)  
   - 🔑 Password WiFi (per la connessione)  
✔️ Richiesta dell'IP del Broker MQTT per la comunicazione  

### 🛠️ 4️⃣ Setup dei File e Caricamento su Arduino  
✔️ Configurazione automatica dei file di sistema  
✔️ Compilazione e upload del codice su Arduino senza necessità di intervento manuale  

🎯 **Risultato:**  
⚡ Arduino è pronto per comunicare in modo sicuro con il broker MQTT utilizzando TLS e autenticazione con API key 🚀  
