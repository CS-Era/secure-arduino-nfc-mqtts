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
  delay(3000); // Ritardo iniziale per poter aprire il Serial Monitor

  Serial.println("\n=== NFCSecure System ===");
  
  // WiFi
  Serial.println("\n[1] Inizializzazione WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connesso!");
  Serial.print("MAC Address: ");
  Serial.println(getMacAddress());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP().toString());
  
  // Test ping
  Serial.println("\n[2] Test ping server...");
  if (WiFi.ping(BROKER_ADDRESS))
      Serial.println("Ping riuscito!");
  else
      Serial.println("Ping fallito!");
  
  // TLS
  Serial.println("\n[3] Setup TLS...");
  wifiClient.setCACert(rootCACert);
  Serial.println("Certificate OK!");
  
  // MQTT
  Serial.println("\n[4] Setup MQTT...");
  mqttClient.setId(getMacAddress());
  mqttClient.setUsernamePassword(getMacAddress(), API_KEY);
  if (!mqttClient.connect(BROKER_ADDRESS, BROKER_PORT)) {
      Serial.println("MQTT connection failed!");
      Serial.print("Error: ");
      Serial.println(mqttClient.connectError());
      while (1);
  }
  Serial.println("MQTT Connected!");
  
  // Inizializzazione componenti
  Serial.println("\n[5] Init components...");
  if (!nfcManager.begin()){
      Serial.println("Errore nell'inizializzazione dei componenti.");
      while (1);
  } else {
      Serial.println("Componenti inizializzati correttamente.");
  }
  
  // Pre-registrazione di un tag valido (UID: 11 22 33 44 55 66 77)
  uint8_t preRegisteredTag[7] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  if (!tagCache.addTag(preRegisteredTag))
    Serial.println("Errore nella registrazione del tag.");
  else
    Serial.println();
  
}

void loop() {
  // Esegui solo 2 round di lettura
  static uint8_t rounds = 0;
  if (rounds < 2) {
      if (nfcManager.update()) {
          rounds++;
      }
  } else {
      Serial.println("\n2 round completati. Stop del loop.");
      while (true) { delay(1000); }
  }
  delay(1000); // Ritardo tra i round
  mqttClient.poll();
}
