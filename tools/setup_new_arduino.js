// tools/setup_new_arduino.js
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const crypto = require('crypto');
const fs = require('fs');
const readline = require('readline');
const { networkInterfaces } = require('os');
require('dotenv').config();

// Funzione per ottenere l'IP locale del computer
function getLocalIP() {
    const nets = networkInterfaces();
    for (const name of Object.keys(nets)) {
        for (const net of nets[name]) {
            // Skip internal (i.e. 127.0.0.1) and non-ipv4 addresses
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

async function getUserInput() {
    console.log('\n=== Configurazione Rete ===');
    console.log('Assicurati che Arduino e questo computer siano sulla stessa rete WiFi\n');

    const ssid = await askQuestion('SSID WiFi: ');
    const password = await askQuestion('Password WiFi: ');
    
    const localIP = getLocalIP();
    console.log(`\nIl tuo IP locale è: ${localIP}`);
    const broker = await askQuestion('Indirizzo IP del broker (premi invio per usare il tuo IP locale): ');
    
    return {
        ssid,
        password,
        broker: broker || localIP
    };
}

async function listArduinoPorts() {
    const ports = await SerialPort.list();
    return ports.filter(port => 
        port.manufacturer && 
        port.manufacturer.toLowerCase().includes('arduino')
    );
}

async function getMacAndSetup() {
    console.log('Cercando Arduino...');
    
    const arduinoPorts = await listArduinoPorts();
    if (arduinoPorts.length === 0) {
        console.error('Nessun Arduino trovato! Collegane uno via USB.');
        process.exit(1);
    }

    console.log(`Arduino trovato su porta: ${arduinoPorts[0].path}`);
    
    // Ottieni input utente
    const networkConfig = await getUserInput();

    const port = new SerialPort({
        path: arduinoPorts[0].path,
        baudRate: 115200
    });

    const parser = port.pipe(new ReadlineParser());

    parser.on('data', async (data) => {
        if (data.includes('Formato per setup_device.js:')) {
            const mac = data.split(':')[1].trim();
            console.log(`\nMAC Address rilevato: ${mac}`);
            
            // Genera API key
            const apiKey = crypto.randomBytes(32).toString('hex');
            
            // Aggiorna .env
            let apiKeys = {};
            if (process.env.API_KEYS) {
                apiKeys = JSON.parse(process.env.API_KEYS);
            }
            apiKeys[mac] = apiKey;
            
            const envContent = `MQTT_PORT=8883\nAPI_KEYS=${JSON.stringify(apiKeys)}`;
            fs.writeFileSync('.env', envContent);
            
            console.log('\nConfigurazione completata:');
            console.log('MAC Address:', mac);
            console.log('API Key:', apiKey);
            console.log('\nFile .env aggiornato!');
            
            // Crea config.h con i valori dell'utente
            const configContent = `
// Network
#define WIFI_SSID "${networkConfig.ssid}"
#define WIFI_PASSWORD "${networkConfig.password}"
#define BROKER_ADDRESS "${networkConfig.broker}"
#define BROKER_PORT 8883

// Security
#define API_KEY "${apiKey}"

// Root CA certificate
const char rootCACert[] PROGMEM = R"EOF(
${fs.readFileSync('certs/ca.crt', 'utf8')}
)EOF";
`;
            fs.writeFileSync('arduino/config.h', configContent);
            console.log('\nFile config.h creato con le tue configurazioni di rete!');
            console.log('\nProssimi passi:');
            console.log('1. Verifica che config.h sia stato creato correttamente');
            console.log('2. Carica lo sketch principale su Arduino');
            console.log('3. Avvia il server MQTT');
            console.log('\nRicorda: Arduino e il server devono essere sulla stessa rete WiFi!');
            
            process.exit(0);
        }
    });
}

console.log('=== Setup Nuovo Arduino ===');
console.log('Questo script configurerà il tuo Arduino per il sistema MQTT/TLS');
getMacAndSetup().catch(console.error);