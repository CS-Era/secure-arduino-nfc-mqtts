// tools/ssl_cert_generator.js
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

function generateCertificates(ipAddress) {
    const certsDir = path.join(__dirname, '..', 'certs');
    
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

module.exports = { generateCertificates };