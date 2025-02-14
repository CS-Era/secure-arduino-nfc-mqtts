#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include "PN532.h"
#include "NFCSecure.h"
#include "config.h"

WiFiSSLClient wifiClient;
MqttClient mqttClient(wifiClient);
NFCReader nfc;
SecureTagCache tagCache;
NFCManager nfcManager(nfc, tagCache, mqttClient);


void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(3000);

  Serial.println("\n=== NFCSecure System ===");
  
  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Connessione stabilita");

  
  
  // TLS
  Serial.println("\n[TLS] Inizializzazione...");
  wifiClient.setCACert(rootCACert);
  Serial.println("[TLS] Certificati caricati");

  
  // MQTT
    Serial.println("\n[MQTT] Inizializzazione...");
    
    // Prepara credenziali cifrate
    String deviceMac = getMacAddress();
    const char* apiKey = API_KEY;
    
    // Cifra e prepara le credenziali
    String encryptedMac = nfcManager.prepareSecureMessage((uint8_t*)deviceMac.c_str(), deviceMac.length());
    String encryptedKey = nfcManager.prepareSecureMessage((uint8_t*)apiKey, strlen(apiKey));
    
    // Usa le credenziali cifrate per l'autenticazione MQTT
    mqttClient.setId("device_" + String(random(0xffff), HEX));
    mqttClient.setUsernamePassword(encryptedMac, encryptedKey);

  if (!mqttClient.connect(BROKER_ADDRESS, BROKER_PORT)) {
      Serial.println("[MQTT] Connessione fallita");
      Serial.println("[MQTT] Esecuzione test diagnostici...");
      
      WiFiClient testClient;
      if (testClient.connect(BROKER_ADDRESS, BROKER_PORT)) {
          Serial.println("[MQTT] Test TCP completato");
          testClient.stop();
      } else {
          Serial.println("[MQTT] Test TCP fallito");
      }
      
      while (1);
  }
  Serial.println("[MQTT] Connessione stabilita");
  
  // Inizializzazione componenti
  if (!nfcManager.begin()){
      Serial.println("[INIT] Errore inizializzazione componenti nfcManager");
      while (1);
  }

  mqttClient.onMessage(onMessageReceived);
  mqttClient.subscribe("nfc/response");
  mqttClient.subscribe("arduino/response");
  Serial.println("[MQTT] Topic configurati");
  
  // Pre-registrazione tag
  uint8_t preRegisteredTag[7] = {0x41, 0xCA, 0xDD, 0x00, 0x00, 0x00, 0x00}; 
  if (!tagCache.addTag(preRegisteredTag))
    Serial.println("[CACHE] Errore registrazione tag");
}


void loop() {
    static bool firstRun = true;
    
    if (firstRun) {
        Serial.println("\n[SYSTEM] In attesa di tag NFC...");
        firstRun = false;
    }

    if (nfcManager.update()) {
        // Aspetta un po' per assicurarci che tutti i messaggi MQTT siano arrivati
        delay(1000);
        
        // Gestisci eventuali messaggi MQTT pendenti
        mqttClient.poll();
        
        // Ora mostra che siamo pronti per un nuovo tag
        Serial.println("\n[SYSTEM] In attesa di tag NFC...");
    }

    mqttClient.poll();
}

String getMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void onMessageReceived(int messageSize) {
  String topic = mqttClient.messageTopic();
  String payload = mqttClient.readString();
  
  Serial.println("\n[MQTT] Messaggio ricevuto");
  Serial.println(payload);
}

// Funzione che accetta un parametro per scegliere quale testo visualizzare
