/**
* server.js
*
* Broker MQTT sicuro con Aedes e TLS per autenticazione e
* verifica degli UID NFC inviati dai client Arduino.
*/

require('dotenv').config({ path: '../.env' });
const fs = require('fs');
const tls = require('tls');
const path = require('path');
const aedes = require('aedes')();
const sqlite3 = require('sqlite3').verbose();
const bcrypt = require('bcrypt');
const crypto = require('crypto');

const saltRounds = 10;

// ---------------------------------------------------------------------
// CRYPTO UTILITY FUNCTIONS
// ---------------------------------------------------------------------

function hexToBuffer(hexString) {
   return Buffer.from(hexString, 'hex');
}

function TEA_decrypt_block_LE(block, key) {
   const v0 = block.readUInt32LE(0);
   const v1 = block.readUInt32LE(4);
   const k0 = key.readUInt32LE(0);
   const k1 = key.readUInt32LE(4);
   const k2 = key.readUInt32LE(8);
   const k3 = key.readUInt32LE(12);

   const delta = 0x9E3779B9;
   let sum = delta * 32;
   let y = v0;
   let z = v1;

   for (let i = 0; i < 32; i++) {
       z = (z - (((y << 4) + k2) ^ (y + sum) ^ ((y >>> 5) + k3))) >>> 0;
       y = (y - (((z << 4) + k0) ^ (z + sum) ^ ((z >>> 5) + k1))) >>> 0;
       sum = (sum - delta) >>> 0;
   }

   const result = Buffer.alloc(8);
   result.writeUInt32LE(y, 0);
   result.writeUInt32LE(z, 4);
   return result;
}

function decrypt(data) {
   if (data.length % 8 !== 0) {
       throw new Error('La lunghezza dei dati deve essere multiplo di 8 byte');
   }

   const key = Buffer.from(process.env.CRYPTO_KEY, 'hex');
   const numBlocks = data.length / 8;
   const decrypted = Buffer.alloc(data.length);

   for (let i = 0; i < numBlocks; i++) {
       const block = data.slice(i * 8, (i + 1) * 8);
       const decryptedBlock = TEA_decrypt_block_LE(block, key);
       decryptedBlock.copy(decrypted, i * 8);
   }

   return decrypted;
}

function removeZeroPadding(buffer) {
   let end = buffer.length;
   while (end > 0 && buffer[end - 1] === 0) {
       end--;
   }
   return buffer.slice(0, end);
}

function generateMAC(data) {
   const key = Buffer.from(process.env.CRYPTO_KEY, 'hex');
   const buffer = Buffer.concat([key, data]);

   let v0 = 0x736f6d65 >>> 0;
   let v1 = 0x646f7261 >>> 0;
   let v2 = 0x6c796765 >>> 0;
   let v3 = 0x74656462 >>> 0;

   for (let i = 0; i < buffer.length; i++) {
       v3 = (v3 ^ buffer[i]) >>> 0;

       v0 = (v0 + v1) >>> 0;
       v1 = ((v1 << 13) | (v1 >>> 19)) >>> 0;
       v1 = (v1 ^ v0) >>> 0;
       v0 = ((v0 << 32) | (v0 >>> 0)) >>> 0;

       v2 = (v2 + v3) >>> 0;
       v3 = ((v3 << 16) | (v3 >>> 16)) >>> 0;
       v3 = (v3 ^ v2) >>> 0;

       v0 = (v0 + v3) >>> 0;
       v3 = ((v3 << 21) | (v3 >>> 11)) >>> 0;
       v3 = (v3 ^ v0) >>> 0;

       v2 = (v2 + v1) >>> 0;
       v1 = ((v1 << 17) | (v1 >>> 15)) >>> 0;
       v1 = (v1 ^ v2) >>> 0;
       v2 = ((v2 << 32) | (v2 >>> 0)) >>> 0;
   }

   const mac = Buffer.alloc(8);
   mac.writeUInt32LE(v0 >>> 0, 0);
   mac.writeUInt32LE(v1 >>> 0, 4);

   return mac;
}

function decryptAndVerify(message) {
   console.log("\n[INFO] Processamento messaggio cifrato");

   const iv = hexToBuffer(message.slice(0, 16));
   const mac = hexToBuffer(message.slice(-16));
   const ciphertext = hexToBuffer(message.slice(16, -16));

   const dataForMac = Buffer.concat([iv, ciphertext]);
   const calculatedMac = generateMAC(dataForMac);

   if (!calculatedMac.equals(mac)) {
       console.log("[SECURITY] Verifica integrità fallita");
       throw new Error('MAC verification failed');
   }

   console.log("[INFO] Verifica integrità completata");

   const plaintext = decrypt(ciphertext);
   const trimmedPlaintext = removeZeroPadding(plaintext);

   return trimmedPlaintext;
}

// ---------------------------------------------------------------------
// CONFIGURAZIONE DEL BROKER TLS CON AEDES
// ---------------------------------------------------------------------

//LOGS: USERNAME, PASSWORD, AUTHSTATE, TOPIC, UIDTAG, TAGSTATE, timestamp, ERROR
//logs: USER,    PASS,     TRUE,     VERIFY,  0000,   TRUE
//logs: USER,    PASS,     FALSE,    FALSE,  NULL,   FALSE
//logs: USER,    PASS,     TRUE,     ACCESS,  0000,   TRUE
//logs: USER,    PASS,     TRUE,     VERIFY,  0000,   FALSE

const CERTS_DIR = path.join(__dirname, '..', 'certs');
const authorizedDevices = JSON.parse(process.env.API_KEYS);
const validUIDs = JSON.parse(process.env.VALID_UIDS);
let logEntry = {};

aedes.authenticate = (client, username, password, callback) => {
    console.log('\n[AUTH] Nuova richiesta di autenticazione');

    const deviceMac = username.toUpperCase();
    const providedApiKey = password.toString();

    logEntry.username = deviceMac;
    logEntry.password = providedApiKey;

    if (authorizedDevices[deviceMac] === providedApiKey) {
        logEntry.auth_state = true;
        console.log('[AUTH] Autenticazione completata con successo');
       callback(null, true);
    } else {
       console.log('[AUTH] Autenticazione fallita');
       callback(new Error('Non autorizzato'), false);
       logEntry.auth_state = false;
       logEntry.topic = null;
       logEntry.uid_tag = null;
       logEntry.tag_state = false;
       logEntry.timestamp = new Date().toISOString();
       logEntry.error = '[AUTH] Autenticazione fallita';
       addLogEntry(logEntry);
    }
};

const serverOptions = {
   key: fs.readFileSync(path.join(CERTS_DIR, 'server.key')),
   cert: fs.readFileSync(path.join(CERTS_DIR, 'server.crt')),
   ca: [fs.readFileSync(path.join(CERTS_DIR, 'ca.crt'))],
   requestCert: false,
   rejectUnauthorized: false,
   minVersion: 'TLSv1.2',
   maxVersion: 'TLSv1.3',
   ciphers: ['ECDHE-RSA-AES128-GCM-SHA256', 'AES128-GCM-SHA256'].join(':')
};

const serverTLS = tls.createServer(serverOptions, aedes.handle);

serverTLS.on('secureConnection', (tlsSocket) => {
   console.log('\n[TLS] Nuova connessione sicura stabilita');
});

aedes.on('client', (client) => {
   console.log('\n[MQTT] Client connesso');
});

aedes.on('clientDisconnect', (client) => {
   console.log('\n[MQTT] Client disconnesso');
});

const portTLS = process.env.MQTT_PORT;
serverTLS.listen(portTLS, () => {
   console.log(`\n[SERVER] Broker MQTT/TLS avviato sulla porta ${portTLS}`);
});

// ---------------------------------------------------------------------
// INIZIALIZZAZIONE DATABASE
// ---------------------------------------------------------------------

const db = new sqlite3.Database('uids.db');

db.serialize(() => {
    db.run(`CREATE TABLE IF NOT EXISTS uids (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       uid TEXT NOT NULL
    )`);

    db.run(`CREATE TABLE IF NOT EXISTS logs (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       username TEXT NOT NULL,
       password TEXT NOT NULL,
       auth_state BOOLEAN FALSE,
       topic TEXT,
       uid_tag TEXT NOT NULL,
       tag_state BOOLEAN FALSE,
       timestamp TEXT NOT NULL,
       error TEXT
    )`);
});

db.get("SELECT COUNT(*) as count FROM uids", (err, row) => {
   if (err) {
       console.error("[DB] Errore nel controllo del database");
   } else if (row.count === 0) {
       console.log("[DB] Inizializzazione database con UID validi");
       validUIDs.forEach(uid => {
           bcrypt.hash(uid, saltRounds, (err, hash) => {
               if (err) {
                   console.error("[DB] Errore nella generazione hash");
                   return;
               }
               db.run("INSERT INTO uids (uid) VALUES (?)", [hash]);
           });
       });
   } else {
       console.log("[DB] Database già inizializzato");
   }
});

function addLogEntry(logEntry) {
    db.serialize(() => {
        const stmt = db.prepare(`INSERT INTO logs (username, password, auth_state, topic, uid_tag, tag_state, timestamp, error) VALUES (?, ?, ?, ?, ?, ?, ?, ?)`);
        stmt.run(
            logEntry.username,
            logEntry.password,
            logEntry.auth_state,
            logEntry.topic,
            logEntry.uid_tag,
            logEntry.tag_state,
            logEntry.timestamp,
            logEntry.error,
            function (err) {
                if (err) {
                    console.error("Errore nell'inserimento del log:", err);
                } else {
                    console.log("Log inserito con ID:", this.lastID);
                }
            }
        );
        stmt.finalize();
    });
}

// ---------------------------------------------------------------------
// VERIFICA UID
// ---------------------------------------------------------------------

function verifyUIDInDB(uid) {
   return new Promise((resolve, reject) => {
       const uidUpperCase = uid.toString('hex').toUpperCase();
       if (validUIDs.some(validUid => validUid.toUpperCase() === uidUpperCase)) {
           resolve(true);
           return;
       }

       db.all("SELECT uid FROM uids", [], (err, rows) => {
           if (err) return reject(err);
           const comparisons = rows.map(row => bcrypt.compare(uid.toString('hex'), row.uid));
           Promise.all(comparisons)
               .then(results => resolve(results.includes(true)))
               .catch(err => reject(err));
       });
   });
}

// ---------------------------------------------------------------------
// GESTIONE MESSAGGI MQTT
// ---------------------------------------------------------------------

aedes.on('publish', (packet, client) => {
   if (!client) return;
   const payloadStr = packet.payload.toString();

   try {
       if (packet.topic === 'nfc/verify' || packet.topic === 'nfc/access') {
           console.log(`\n[MQTT] Messaggio ricevuto su "${packet.topic}"`);

           const uid = decryptAndVerify(payloadStr);
           logEntry.uid_tag = uid;

           if (packet.topic === 'nfc/verify') {
               logEntry.topic = 'nfc/verify';
               verifyUIDInDB(uid)
                   .then(isValid => {
                       if (isValid) {
                           logEntry.tag_state = true;
                           console.log("[ACCESS] Verifica UID completata con successo");
                           aedes.publish({
                               topic: 'nfc/response',
                               payload: Buffer.from("[SUCCESS] Tag NFC verificato e autorizzato!")
                           });
                       } else {
                           logEntry.tag_state = false;
                           console.log("[ACCESS] Verifica UID fallita");
                           aedes.publish({
                               topic: 'nfc/response',
                               payload: Buffer.from("[ERROR] Tag NFC non autorizzato!")
                           });
                       }
                   })
                   .catch(err => {
                       logEntry.error = err;
                       console.error("[ERROR] Errore durante la verifica");
                       aedes.publish({
                           topic: 'nfc/response',
                           payload: Buffer.from("[ERROR] Errore nella verifica del tag")
                       });
                   });
               logEntry.timestamp = new Date().toISOString();
           }
           else if (packet.topic === 'nfc/access') {
               logEntry.topic = 'nfc/access';
               logEntry.tag_state = true;
               const timestamp = new Date().toISOString();
               console.log(`[ACCESS] Nuovo accesso registrato al ${timestamp}`);
               aedes.publish({
                   topic: 'nfc/response',
                   payload: Buffer.from("[LOG] Evento di accesso registrato nei log del server")
               });
               logEntry.timestamp = new Date().toISOString();
           }
       }
   } catch (error) {
       logEntry.error = error;
       console.error("[ERROR] Errore nel processamento del messaggio");
       aedes.publish({
           topic: 'nfc/response',
           payload: Buffer.from("[ERROR] Errore nel processare la richiesta")
       });
   }
    addLogEntry(logEntry);
});
