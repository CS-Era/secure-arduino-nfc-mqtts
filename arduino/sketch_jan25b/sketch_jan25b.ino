/*************************************************************
  mqtt_secure_client.ino
  MQTT su TLS con autenticazione device
*************************************************************/

#include <WiFiS3.h>
#include <ArduinoMqttClient.h>

/***************************************************
  Inserisci il certificato CA self-signed
  generato su Ubuntu (ca.crt)
****************************************************/
const char rootCACert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDKzCCAhOgAwIBAgIUY8p/xdlNb4LsgWcu6wq8nqJ+2uowDQYJKoZIhvcNAQEL
BQAwJTELMAkGA1UEBhMCSVQxFjAUBgNVBAMMDTE5Mi4xNjguMS4xMDMwHhcNMjUw
MTI1MTczNTEwWhcNMjYwMTI1MTczNTEwWjAlMQswCQYDVQQGEwJJVDEWMBQGA1UE
AwwNMTkyLjE2OC4xLjEwMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
AMXfPH7/YGaEsZAo1Y//PtefnNSj+SbpAiB0iy9XKriJ+k58OdVuW12ayU/solnm
n8NsSwGj0yyGZ6v116+anR+mI9dKEcqcLmg/KNz24+8QXe9T9E5QLM9EXd2EMSTw
ptIRQSgCGit1YH3MA9sz9iIb8LXOzePBPQEAI3Uskn7pS5HArfYRMbDbrs+hMgQj
IQ8a9lvoq39A64jVcUwNa1ousu5OZdFEBtdthSjvWibCq70v5LXnrRiaGT8qRj++
5oz20D11hQHe4FuYZ65toWjxTOzllp4wZY2+yCedB+FFc0H/+SgbFc3/t4qD1MNM
iiMBDaydk1ea4dw7FjeaZ6kCAwEAAaNTMFEwHQYDVR0OBBYEFELsr1GuzooLjLen
3/C50EKtjZu6MB8GA1UdIwQYMBaAFELsr1GuzooLjLen3/C50EKtjZu6MA8GA1Ud
EwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAMSiTqi1LOpRys/uZfKJ3z9z
Noaky9HgccOBO+SozOgheoChfyg2vwy24fK28TyY3AYDoxWgR/yJzM/3oNjCmVV6
kKiLvw/C07a2fNlwfp4OEaYnq8PQQCcrz4SFCQO2O6lJLi9dSjvjcdnEyodyrQR0
O3+T98BsR+svxnll9Bhfw9aYIA/AGk8fzSDeFfSGGV86CyGNFiYMBViguIdMvBRq
C5HRU27wTa6lgOQFdT53lpnVoHH1WDwkVOVPJYySRnUXrBzvprTtySSb8S5n0Klu
8J0fETvo2gpg8JLcUTUQy4CnBBgXlEm2I3QMhYpFxFfLKPUoVkAezkCwnVPMbVs=
-----END CERTIFICATE-----
)EOF";

// Credenziali WiFi
const char* ssid = "Cameretta_WiFi";
const char* password = "PasswordWifiCasa255!!";

// Parametri broker MQTT
const char* brokerAddress = "192.168.1.103";
const int brokerPort = 8883;

// API Key unica per questo dispositivo
const char* API_KEY = "3a1c5b2f8d4e9a7c6b2d5e8f3a1c9b4d";

// Client WiFi e MQTT
WiFiSSLClient wifiClient;
MqttClient mqttClient(wifiClient);

// Topic MQTT
const char* pubTopic = "test/topic";
const char* subTopic = "test/topic";

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
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
       delay(500);
       Serial.print(".");
   }
   Serial.println("\nWiFi connesso!");
   
   // Mostra info dispositivo
   Serial.print("MAC Address: ");
   Serial.println(getMacAddress());
   Serial.print("IP Address: ");
   Serial.println(WiFi.localIP());

   // 2. Test preliminare connettività
   Serial.println("\n[2] Test ping server...");
   if(WiFi.ping(brokerAddress)) {
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
   mqttClient.setId(getMacAddress().c_str());
   mqttClient.setUsernamePassword(getMacAddress().c_str(), API_KEY);
   
   Serial.println("Tentativo connessione al broker...");
   if (!mqttClient.connect(brokerAddress, brokerPort)) {
       Serial.println("❌ Connessione fallita!");
       Serial.print("Errore: ");
       Serial.println(mqttClient.connectError());
       while (1);
   }

   Serial.println("✅ Connesso al broker MQTT!");

   // 5. Subscribe e test
   Serial.println("\n[5] Test comunicazione...");
   mqttClient.onMessage(onMqttMessage);
   mqttClient.subscribe(subTopic);
   
   mqttClient.beginMessage(pubTopic);
   mqttClient.print("Ciao dal dispositivo: " + getMacAddress());
   mqttClient.endMessage();
   
   Serial.println("Messaggio di test inviato");
}

void loop() {
   mqttClient.poll();
   delay(1000);
}