/**
 * @file server.js
 * @brief MQTT broker per l'autenticazione e il logging sicuro di dispositivi nfc.
 * @description Questo file implementa un broker MQTT sicuro utilizzando Aedes e TLS. 
 *              I Client Arduino vengono autenticati tramite un registro basato su MAC Address e API Key.
 *              In questo modo, superiamo l'ostacolo dell'impossibilità di implementare mTLS a causa della 
 *              libreria WiFiS3 di Arduino.
 */

require('dotenv').config();
const fs = require('fs'); // Modulo per la gestione dei file (lettura delle chiavi TLS)
const tls = require('tls'); // Modulo per la gestione delle connessioni TLS
const aedes = require('aedes')(); // Istanza di Aedes, un broker MQTT leggero

/**
 * @brief Registro dei dispositivi Arduino autorizzati
 * @description Mappa di MAC Address autorizzati con la rispettiva API Key.
 */
const authorizedDevices = JSON.parse(process.env.API_KEYS);

/**
 * @brief Configurazione del server TLS
 * @description Definisce le opzioni di sicurezza per il server MQTT con TLS.
 */
const serverOptions = {
    key: fs.readFileSync(process.env.SERVER_KEY_PATH), // Chiave privata del server
    cert: fs.readFileSync(process.env.SERVER_CERT_PATH), // Certificato TLS del server
    ca: [fs.readFileSync(process.env.CA_CERT_PATH)], // Certificato dell'Autorità di Certificazione (CA)

    requestCert: false, // Non richiede certificati client (no mTLS)
    rejectUnauthorized: false, 

    minVersion: 'TLSv1.2', // Forza l'uso di TLS 1.2 per compatibilità con Arduino
    maxVersion: 'TLSv1.3', 

    ciphers: [
        'ECDHE-RSA-AES128-GCM-SHA256',
        'AES128-GCM-SHA256' 
    ].join(':')
};

/**
 * @brief Funzione di autenticazione MQTT
 * @description Verifica le credenziali del dispositivo utilizzando MAC Address e API Key.
 * @param {Object} client - Il client MQTT che sta tentando la connessione.
 * @param {string} username - Il MAC Address fornito come identificativo del dispositivo.
 * @param {Buffer} password - La API Key fornita dal client per l'autenticazione.
 * @param {Function} callback - Funzione di callback per restituire il risultato dell'autenticazione.
 * @returns {void}
 */
aedes.authenticate = (client, username, password, callback) => {
    console.log('\n🔍 Tentativo di autenticazione:');
    console.log('- Device MAC:', username);

    const deviceMac = username.toUpperCase(); // Normalizzazione del MAC Address
    const providedApiKey = password.toString(); 

    if (authorizedDevices[deviceMac] === providedApiKey) {
        console.log('✅ Dispositivo IoT autenticato con successo!');
        callback(null, true);
    } else {
        console.log('❌ Autenticazione fallita');
        console.log('- MAC non autorizzato o API key non valida');
        callback(new Error('Non autorizzato'), false);
    }
};

/**
 * @brief Creazione del server TLS per MQTT
 * @description Inizializza il server TLS utilizzando la configurazione specificata.
 */
const serverTLS = tls.createServer(serverOptions, aedes.handle);

/**
 * @brief Gestione delle connessioni sicure TLS
 * @description Questo evento viene attivato ogni volta che un nuovo client si connette in modo sicuro.
 * @param {Object} tlsSocket - Il socket TLS della connessione stabilita.
 */
serverTLS.on('secureConnection', (tlsSocket) => {
    console.log('\n🔒 Nuova connessione TLS stabilita:');
    console.log('- Protocollo TLS:', tlsSocket.getProtocol());
    console.log('- Cipher Suite:', tlsSocket.getCipher());
});

/**
 * @brief Evento di connessione di un client MQTT
 * @description Notifica quando un nuovo client MQTT si connette al broker.
 * @param {Object} client - Il client MQTT connesso.
 */
aedes.on('client', (client) => {
    console.log('\n📥 Client MQTT connesso:');
    console.log('- ID:', client.id);
});

/**
 * @brief Evento di disconnessione di un client MQTT
 * @description Notifica quando un client MQTT si disconnette dal broker.
 * @param {Object} client - Il client MQTT disconnesso.
 */
aedes.on('clientDisconnect', (client) => {
    console.log('\n📤 Client MQTT disconnesso:');
    console.log('- ID:', client.id);
});

/**
 * @brief Avvio del server MQTT su TLS
 * @description Il server ascolta sulla porta 8883 (standard MQTT su TLS).
 */
const portTLS = 8883;
serverTLS.listen(portTLS, () => {
    console.log(`\n🚀 Broker MQTT/TLS in ascolto sulla porta ${portTLS}`);
});

/**
 * @section TODO Funzionalità da aggiungere 
 * 1. Implementare una classe di gestione dei tag NFC, e nello specifico funzionalità riguardo a:
 *    - Gestione di richieste di registrazione di un nuovo tag (tag ID) da un certo device (MAC) 
 *    - Approvazione dei tag per cui è stata richiesta la registrazione in base al tag ID.
 *    - Verifica se un tag ID è autorizzato ad accedere
 * 2. Creare topic MQTT appositi per nfc/auth, nfc/auth/response, nfc/register ecc.
 * 3. Creare funzione di gestione dei messaggi MQTT per NFC
 * 4. Creare un sistema di logging che copra per intero il ciclo di vita del server:
 *    - Nuove connessioni, da quale device, topic utilizzati
 *    - Richieste di registrazione di tag, con indicazioni dei timestamp e del device richiedente
 *    - Tag approvati, richieste di autenticazione, successo/insuccesso dell'operazione, timestamp, device richiedente
 * 5. Cifratura ed autenticazione dei log per garantire confidenzialità e integrità
 *    - il salvataggio può avvenire come file sul server o possiamo integrare un database 
 */