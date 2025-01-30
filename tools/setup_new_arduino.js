// tools/setup_new_arduino.js
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const { execSync } = require('child_process');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');
const readline = require('readline');
const { networkInterfaces } = require('os');

// Configurazione percorsi relativi dalla cartella tools
const PROJECT_ROOT = path.join(__dirname, '..');
const PATHS = {
    ENV_EXAMPLE: path.join(PROJECT_ROOT, '.env.example'),
    ENV: path.join(PROJECT_ROOT, '.env'),
    CONFIG_EXAMPLE: path.join(PROJECT_ROOT, 'arduino', 'sketch_jan25b', 'config.h.example'),
    CONFIG: path.join(PROJECT_ROOT, 'arduino', 'sketch_jan25b', 'config.h'),
    CERTS_DIR: path.join(PROJECT_ROOT, 'certs'),
    SKETCH: path.join(PROJECT_ROOT, 'arduino', 'sketch_jan25b', 'sketch_jan25b.ino')
};

// Funzione per ottenere l'IP locale del computer
function getLocalIP() {
    const nets = networkInterfaces();
    for (const name of Object.keys(nets)) {
        for (const net of nets[name]) {
            if (net.family === 'IPv4' && !net.internal) {
                return net.address;
            }
        }
    }
    return 'localhost';
}

// Funzione per input utente
function askQuestion(query) {
    const rl = readline.createInterface({
        input: process.stdin,
        output: process.stdout,
    });

    return new Promise(resolve => rl.question(query, ans => {
        rl.close();
        resolve(ans);
    }));
}

// Funzione per verificare l'esistenza dei file necessari
function checkPrerequisites() {
    console.log('\n🔍 Verifico i prerequisiti...');
    
    const requiredFiles = [
        PATHS.ENV_EXAMPLE,
        PATHS.CONFIG_EXAMPLE,
        PATHS.SKETCH
    ];

    for (const file of requiredFiles) {
        if (!fs.existsSync(file)) {
            throw new Error(`File ${file} non trovato`);
        }
    }

    // Verifica OpenSSL
    try {
        execSync('openssl version');
        console.log('✅ OpenSSL trovato nel sistema');
    } catch (error) {
        throw new Error('OpenSSL non trovato. Installalo prima di continuare.');
    }

    console.log('✅ Tutti i prerequisiti sono soddisfatti');
}

// Funzione per generare i certificati SSL
async function generateCertificates(ipAddress) {
    const certsDir = PATHS.CERTS_DIR;
    
    // Crea la directory certs se non esiste
    if (!fs.existsSync(certsDir)) {
        fs.mkdirSync(certsDir);
    }

    try {
        console.log('\n🔐 Generazione certificati SSL...');

        // 1. Genera CA key e certificato
        console.log('Generazione CA...');
        execSync(`openssl req -x509 -newkey rsa:4096 -nodes -days 365 -keyout "${path.join(certsDir, 'ca.key')}" -out "${path.join(certsDir, 'ca.crt')}" -subj "/C=IT/CN=${ipAddress}"`, { stdio: 'inherit' });

        // 2. Genera server key e CSR
        console.log('Generazione server key e CSR...');
        execSync(`openssl req -newkey rsa:4096 -nodes -keyout "${path.join(certsDir, 'server.key')}" -out "${path.join(certsDir, 'server.csr')}" -subj "/C=IT/CN=${ipAddress}"`, { stdio: 'inherit' });

        // 3. Firma il certificato server con la CA
        console.log('Firma del certificato server...');
        execSync(`openssl x509 -req -in "${path.join(certsDir, 'server.csr')}" -CA "${path.join(certsDir, 'ca.crt')}" -CAkey "${path.join(certsDir, 'ca.key')}" -CAcreateserial -out "${path.join(certsDir, 'server.crt')}" -days 365`, { stdio: 'inherit' });

        console.log('✅ Certificati generati con successo');
        return true;

    } catch (error) {
        console.error('❌ Errore nella generazione dei certificati:', error.message);
        throw error;
    }
}

// Funzione per inizializzare .env se non esiste
function initEnvFile() {
    console.log('\n🔧 Inizializzazione file .env...');
    
    if (!fs.existsSync(PATHS.ENV)) {
        let envContent = fs.readFileSync(PATHS.ENV_EXAMPLE, 'utf8');
        // Inizializza con un oggetto API_KEYS vuoto
        envContent = envContent.replace(/API_KEYS=\{.*\}/, 'API_KEYS={}');
        fs.writeFileSync(PATHS.ENV, envContent);
        console.log('✅ File .env creato da .env.example');
    } else {
        console.log('ℹ️  File .env già esistente');
    }
}

// Funzione per acquisire il MAC Address dall'Arduino
async function getMacFromArduino() {
    console.log('\n🔍 Ricerca Arduino...');
    
    const ports = await SerialPort.list();
    const arduinoPorts = ports.filter(port => 
        port.manufacturer && 
        port.manufacturer.toLowerCase().includes('arduino')
    );

    if (arduinoPorts.length === 0) {
        throw new Error('Nessun Arduino trovato! Collegane uno via USB.');
    }

    const portPath = arduinoPorts[0].path;
    console.log(`✅ Arduino trovato su porta: ${portPath}`);

    console.log('\n⚠️  Per ottenere il MAC Address:');
    console.log('1. Copia questo sketch in Arduino IDE:');
    console.log(`
#include <WiFiS3.h>
void setup() {
    Serial.begin(115200);
    while (!Serial);
    byte mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(macStr);
}
void loop() {
    delay(1000);
}`);
    console.log('2. Caricalo sull\'Arduino');
    console.log('3. Premi INVIO quando sei pronto a leggere il MAC');
    
    await askQuestion('\nPremi INVIO quando hai caricato lo sketch...');

    return new Promise((resolve, reject) => {
        const port = new SerialPort({
            path: portPath,
            baudRate: 115200
        });

        const parser = port.pipe(new ReadlineParser());
        let timeoutId;

        timeoutId = setTimeout(() => {
            port.close();
            reject(new Error('Timeout nella lettura del MAC Address'));
        }, 10000);

        parser.on('data', (data) => {
            const mac = data.trim().toUpperCase();
            if (mac.match(/^([0-9A-F]{2}:){5}[0-9A-F]{2}$/)) {
                clearTimeout(timeoutId);
                port.close();
                resolve(mac);
            }
        });

        port.on('error', (err) => {
            clearTimeout(timeoutId);
            reject(err);
        });
    });
}

// Funzione per ottenere input di configurazione dall'utente
async function getNetworkConfig() {
    console.log('\n⚙️  Configurazione rete');
    console.log('Assicurati che Arduino e questo computer siano sulla stessa rete WiFi\n');

    const ssid = await askQuestion('SSID WiFi: ');
    if (!ssid) throw new Error('SSID non può essere vuoto');

    const password = await askQuestion('Password WiFi: ');
    if (!password) throw new Error('Password non può essere vuota');
    
    const localIP = getLocalIP();
    console.log(`\nIl tuo IP locale è: ${localIP}`);
    const broker = await askQuestion('Indirizzo IP del broker (premi invio per usare il tuo IP locale): ');
    
    return {
        ssid,
        password,
        broker: broker || localIP
    };
}

// Funzione principale
async function main() {
    try {
        console.log('=== Setup Nuovo Arduino ===');
        console.log('Questo script configurerà il tuo Arduino per il sistema MQTT/TLS\n');

        // 1. Verifica prerequisiti
        checkPrerequisites();

        // 2. Ottieni configurazione rete (prima, perché ci serve l'IP per i certificati)
        const networkConfig = await getNetworkConfig();

        // 3. Genera certificati SSL
        await generateCertificates(networkConfig.broker);

        // 4. Inizializza .env
        initEnvFile();

        // 5. Ottieni MAC Address
        const mac = await getMacFromArduino();
        console.log(`\n✅ MAC Address rilevato: ${mac}`);

        // 6. Genera API key
        const apiKey = crypto.randomBytes(16).toString('hex');
        console.log(`\n🔑 API Key generata: ${apiKey}`);

        // 7. Aggiorna .env con percorsi certificati e API key
        console.log('\n📝 Aggiornamento file .env...');
        let envContent = fs.readFileSync(PATHS.ENV, 'utf8');
        const apiKeysMatch = envContent.match(/API_KEYS=({.*})/);
        let apiKeys = apiKeysMatch ? JSON.parse(apiKeysMatch[1]) : {};
        apiKeys[mac] = apiKey;
        
        envContent = envContent
            .replace(/API_KEYS=\{.*\}/, `API_KEYS=${JSON.stringify(apiKeys)}`)
            .replace('../certs/server.key', path.join('certs', 'server.key'))
            .replace('../certs/server.crt', path.join('certs', 'server.crt'))
            .replace('../certs/ca.crt', path.join('certs', 'ca.crt'));
        
        fs.writeFileSync(PATHS.ENV, envContent);
        console.log('✅ File .env aggiornato');

        // 8. Crea config.h dal template
        console.log('\n📝 Creazione config.h dal template...');
        let configContent = fs.readFileSync(PATHS.CONFIG_EXAMPLE, 'utf8');
        const caCert = fs.readFileSync(path.join(PATHS.CERTS_DIR, 'ca.crt'), 'utf8');
        
        // Sostituisci i valori nel template
        configContent = configContent
            .replace(/your_wifi_ssid/g, networkConfig.ssid)
            .replace(/your_wifi_password/g, networkConfig.password)
            .replace(/your_server_or_local_ip_address/g, networkConfig.broker)
            .replace(/your_api_key/g, apiKey)
            .replace(/your server CA certificate/g, caCert.trim());
        
        fs.writeFileSync(PATHS.CONFIG, configContent);
        console.log('✅ File config.h creato dal template');

        console.log('\n✅ Setup completato con successo!');
        console.log('\nProssimi passi:');
        console.log('1. Verifica che config.h sia stato creato correttamente');
        console.log('2. Carica lo sketch principale su Arduino');
        console.log('3. Avvia il server MQTT');
        console.log('\nRicorda: Arduino e il server devono essere sulla stessa rete WiFi!');

    } catch (error) {
        console.error('\n❌ Errore durante il setup:', error.message);
        process.exit(1);
    }
}

// Avvio dello script
main().catch(console.error);