#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include "MockPN532.h"
#include "NFCSecure.h"
#include "config.h"

WiFiSSLClient wifiClient;
MqttClient mqttClient(wifiClient);
MockPN532 nfc;
SecureTagCache tagCache;
NFCManager nfcManager(nfc, tagCache, mqttClient);

String getMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(3000);

  Serial.println("\n=== NFCSecure System ===");
  
  // WiFi
  Serial.println("\n[WIFI] Inizializzazione...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Connessione stabilita");
  Serial.println("[WIFI] Device inizializzato");
  
  // Test ping
  Serial.println("\n[CONN] Test connettività server...");
  if (WiFi.ping(BROKER_ADDRESS))
      Serial.println("[CONN] Test completato con successo");
  else
      Serial.println("[CONN] Test fallito");
  
  // TLS
  Serial.println("\n[TLS] Inizializzazione sicurezza...");
  wifiClient.setCACert(rootCACert);
  Serial.println("[TLS] Certificati caricati");

  Serial.println("\n[CONN] Test porta sicura...");
  WiFiClient testClient;
  if (testClient.connect(BROKER_ADDRESS, BROKER_PORT)) {
      Serial.println("[CONN] Porta raggiungibile");
      testClient.stop();
  } else {
      Serial.println("[CONN] Porta non raggiungibile");
  }
  
  // MQTT
  Serial.println("\n[MQTT] Inizializzazione...");
  mqttClient.setId(getMacAddress());
  mqttClient.setUsernamePassword(getMacAddress(), API_KEY);

  Serial.println("[MQTT] Tentativo di connessione...");

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
  Serial.println("\n[INIT] Setup componenti...");
  if (!nfcManager.begin()){
      Serial.println("[INIT] Errore inizializzazione");
      while (1);
  } else {
      Serial.println("[INIT] Completata con successo");
  }

  Serial.println("\n[MQTT] Configurazione topic...");
  mqttClient.onMessage(onMessageReceived);
  mqttClient.subscribe("nfc/response");
  mqttClient.subscribe("arduino/response");
  Serial.println("[MQTT] Topic configurati");
  
  // Pre-registrazione tag
  uint8_t preRegisteredTag[7] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  if (!tagCache.addTag(preRegisteredTag))
    Serial.println("[CACHE] Errore registrazione tag");
  else
    Serial.println("[CACHE] Tag registrato con successo");
}

void onMessageReceived(int messageSize) {
  String topic = mqttClient.messageTopic();
  String payload = mqttClient.readString();
  
  Serial.println("\n[MQTT] Messaggio ricevuto");
  Serial.println(payload);
}

void loop() {
  static uint8_t rounds = 0;
  if (rounds < 2) {
      if (nfcManager.update()) {
          rounds++;
      }
  } else {
      Serial.println("\n[SYS] Test completati");
      while (true) { delay(1000); }
  }
  delay(1000);
  mqttClient.poll();
}