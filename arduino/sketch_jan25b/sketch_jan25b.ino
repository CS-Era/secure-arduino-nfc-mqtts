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


//inizio biagio
#include <PubSubClient.h>
#include "mbedtls/aes.h"

// Chiave AES (deve essere la stessa sul server)
const byte aes_key[16] = { 
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF 
};

// Vettore di Inizializzazione (IV) (deve essere inviato con il messaggio)
byte iv[16] = { 
  0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
  0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 
};

void encryptAES(byte *input, byte *output) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, aes_key, 128);
    
    byte temp_iv[16];
    memcpy(temp_iv, iv, 16); // Copia dell'IV per non modificarlo
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 16, temp_iv, input, output);
    
    mbedtls_aes_free(&aes);
}

void sendEncryptedUID() {
    String uid = "P9Q3W7E1R5T2Y6U4"; // UID di esempio
    byte plainText[16] = {0};
    byte encryptedText[16] = {0};
    
    memcpy(plainText, uid.c_str(), uid.length());
    
    encryptAES(plainText, encryptedText);

    char encryptedBase64[25];
    for (int i = 0; i < 16; i++) {
        sprintf(encryptedBase64 + (i * 2), "%02X", encryptedText[i]);
    }

    client.publish("arduino/uid", encryptedBase64);
    Serial.print("UID criptato inviato: ");
    Serial.println(encryptedBase64);
}


//fine biagio


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
