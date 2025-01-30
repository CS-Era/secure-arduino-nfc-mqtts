/*************************************************************
  mqtt_secure_client.ino
  MQTT su TLS con autenticazione device
*************************************************************/

#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include "config.h"  // Include il file di configurazione

// SECURITY: Verifica presenza file di configurazione
#ifndef CONFIG_H
  #error "Manca il file config.h. Rinomina config.h.example in config.h e inserisci le tue credenziali"
#endif

// Client WiFi e MQTT
WiFiSSLClient wifiClient;
MqttClient mqttClient(wifiClient);

// Ottieni MAC address come stringa
String getMacAddress() {
   byte mac[6];
   WiFi.macAddress(mac);
   char macStr[18];
   snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
   return String(macStr);
}


// Callback per i messaggi in arrivo
void onMqttMessage(int messageSize) {
   String topic = mqttClient.messageTopic();
   // SECURITY: Limita la dimensione del messaggio per prevenire overflow
   const size_t maxMessageSize = 1024;
   if (messageSize > maxMessageSize) {
       Serial.println("Messaggio troppo grande, ignorato");
       return;
   }
   
   String message = mqttClient.readString();
   
   Serial.println("\nMessaggio ricevuto:");
   Serial.println("- Topic: " + topic);
   Serial.println("- Contenuto: " + message);
}

void setup() {
   Serial.begin(115200);
   while (!Serial);

   Serial.println("\n\n--- Debug Connessione MQTT/TLS con Device Auth ---");

   // 1. WiFi
   Serial.println("\n[1] Inizializzazione WiFi...");
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   
   // SECURITY: Timeout per la connessione WiFi
   unsigned long startAttemptTime = millis();
   while (WiFi.status() != WL_CONNECTED && 
          millis() - startAttemptTime < 20000) {  // 20 secondi timeout
       delay(500);
       Serial.print(".");
   }
   
   if (WiFi.status() != WL_CONNECTED) {
       Serial.println("\nImpossibile connettersi al WiFi. Riavvio...");
       delay(1000);
       NVIC_SystemReset();  // Reset sicuro del dispositivo
   }
   
   Serial.println("\nWiFi connesso!");
   
   // Mostra info dispositivo
   Serial.print("MAC Address: ");
   Serial.println(getMacAddress());
   Serial.print("IP Address: ");
   Serial.println(WiFi.localIP());

   // 2. Test preliminare connettività
   Serial.println("\n[2] Test ping server...");
   if(WiFi.ping(BROKER_ADDRESS)) {
       Serial.println("Ping riuscito!");
   } else {
       Serial.println("Ping fallito!");
   }

   // 3. Setup TLS
   Serial.println("\n[3] Setup TLS...");
   wifiClient.stop();
   delay(1000);
   
   Serial.println("Caricamento certificato CA...");
   wifiClient.setCACert(NULL);
   delay(100);
   wifiClient.setCACert(rootCACert);
   Serial.println("Certificato CA caricato");

// 4. Connessione MQTT con autenticazione
   Serial.println("\n[4] Connessione MQTT con autenticazione...");
   
   // SECURITY: Imposta timeout per la connessione MQTT
   mqttClient.setConnectionTimeout(10000);  // 10 secondi
   
   // Ottieni MAC address e imposta credenziali
   String macAddress = getMacAddress();
   Serial.print("MAC Address usato per auth: ");
   Serial.println(macAddress);
   
   mqttClient.setId(macAddress);
   mqttClient.setUsernamePassword(macAddress.c_str(), API_KEY);
   
   Serial.println("Tentativo connessione al broker...");
   if (!mqttClient.connect(BROKER_ADDRESS, BROKER_PORT)) {
       Serial.println("❌ Connessione fallita!");
       Serial.print("Errore: ");
       Serial.println(mqttClient.connectError());
       delay(1000);
       NVIC_SystemReset();  // Reset sicuro del dispositivo
   }
   
   Serial.println("✅ Connesso al broker MQTT!");

   // 5. Subscribe e test
   Serial.println("\n[5] Test comunicazione...");
   mqttClient.onMessage(onMqttMessage);
   mqttClient.subscribe(SUB_TOPIC);
   
   mqttClient.beginMessage(PUB_TOPIC);
   mqttClient.print("Ciao dal dispositivo: " + getMacAddress());
   mqttClient.endMessage();
   
   Serial.println("Messaggio di test inviato");
}

void loop() {
   // SECURITY: Verifica periodicamente lo stato della connessione
   if (!mqttClient.connected() || WiFi.status() != WL_CONNECTED) {
       Serial.println("Connessione persa. Riavvio...");
       delay(1000);
       NVIC_SystemReset();  // Reset sicuro del dispositivo
   }
   
   mqttClient.poll();
   delay(1000);
}