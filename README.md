# 🔒 IoT Security Project - NFC Access Control System

Secure access control system based on **Arduino UNO R4 WiFi** with **NFC PN532** reader and **MQTT server with TLS**. The project implements a two-level authentication system that combines local and remote verification of NFC tags with encrypted and secure communications.

---
## 📌 Prerequisites
🔹 **Hardware**  
- 🛠️ Arduino UNO R4 WiFi  
- 📡 NFC module PN532
- 🔌 Jumper cables for connections
- 🔌 USB-C cable  

🔹 **Software**  
- 🖥️ Node.js and npm  
- ⚙️ Arduino IDE  

🔹 **Libraries**
- 📡 **ArduinoMqttClient** by Arduino  
- 🔄 **Adafruit BusIO** by Adafruit  
- 🏷️ **Adafruit PN532** by Adafruit 
- 📑 **ArduinoJson** by Benoît Blanchnon  

Also put the libraries present in `arduino/pn532_libraries` in the libraries folder of Arduino

---

## 🚀 Project Setup

### 1️⃣ Clone the repository
```bash
git clone https://github.com/CS-Era/secure-arduino-nfc-mqtts.git
cd secure-arduino-nfc-mqtts
```

### 2️⃣ Install server dependencies
```bash
cd server
npm install
```
### 2️⃣ Install tools dependencies
```bash
cd ../tools
npm install
```
### 3️⃣ Start the arduino setup file
```bash
node setup_new_arduino.js
```
### 4️⃣ Start the MQTT server
```bash
node server/server.js
```
   

## ⚙️ Features of the Setup Script

The automated setup performs several tasks to configure Arduino UNO R4 WiFi and the MQTT server in just a few steps  

### 🔍 1️⃣ Detection and Identification.  
✔️ Automatic recognition of the connected Arduino board  

### 🔑 2️⃣ Secure Credential Generation.  
✔️ Creation of a unique and secure API key for device authentication.  

### 📶 3️⃣ Network Configuration.  
✔️ Interactive request for WiFi credentials:  
   - 📡 SSID (WiFi network name).  
   - 🔑 WiFi password (for connection)  
✔️ Requesting the IP of the MQTT Broker for communication  

### 🛠️ 4️⃣ File Setup
✔️ Automatic configuration of system .env and config.h files.  
  
