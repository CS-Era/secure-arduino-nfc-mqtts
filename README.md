# IoT Security Project - MQTT over TLS

Sistema di comunicazione sicura tra Arduino UNO R4 WiFi e server MQTT con TLS e autenticazione dei dispositivi.

## Prerequisiti
- Arduino UNO R4 WiFi
- Node.js e npm
- Arduino IDE

## Librerie Arduino
- ArduinoMqttClient by Arduino
- Adafruit BusIO by Adafruit
- Adafruit PN532 by Adafruit
- ArduinoJson by Benoit Blanchnon
- PubSubClient by Nick O'Leary

## Setup

1. Clona il repository
```bash
git clone https://github.com/CS-Era/iot-security-project.git
cd iot-security-project
```

2. Installa le dipendenze
```bash
npm install
```
   
3. Collega Arduino UNO R4 WiFi al computer via USB-C

4. Esegui lo script di setup automatico
```bash
node tools/setup_new_arduino.js
```

5. Avvia il server
```bash
node server/server.js
```
   

## Cosa fa lo script
* Rilevamento arduino
* Identificazione MAC Address
* Generazione API key sicura
* Richiesta informazioni di rete:
  - SSID WiFi
  - Password WiFi
  - IP del Broker
* Configurazione di tutti i file necessari
* Caricamento del codice su Arduino
