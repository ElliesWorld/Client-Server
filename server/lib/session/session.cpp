#include "session.h"
#include "communication.h"

// Define the hardcoded HMAC secret key
const char *Session::HMAC_SECRET_KEY = "Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+";

// LED Pin Definition
const int LED_PIN = 21;

Session::Session() : sessionEstablished(false), lastActivityTime(0)
{
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize cryptographic contexts
    initializeCryptoContexts();
}

Session::~Session()
{
    // Cleanup resources
    cleanupCryptoContexts();
}

void Session::initializeCryptoContexts()
{
    // Initialize cryptographic contexts
    mbedtls_pk_init(&serverRsaKey);
    mbedtls_pk_init(&clientRsaKey);
    mbedtls_aes_init(&aesContext);
    mbedtls_ctr_drbg_init(&ctrDrbg);
    mbedtls_entropy_init(&entropy);

    // Seed the random number generator
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbg);

    if (mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy, NULL, 0) != 0)
    {
        // Handle seeding error
        indicateSessionFailed();
    }
}

void Session::cleanupCryptoContexts()
{
    // Free cryptographic resources
    mbedtls_pk_free(&serverRsaKey);
    mbedtls_pk_free(&clientRsaKey);
    mbedtls_aes_free(&aesContext);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_entropy_free(&entropy);
}

bool Session::establishKeyExchange()
{

    Communication comm(Serial);

    // Turn off LED at start of key exchange
    digitalWrite(LED_PIN, LOW);

    try
    {
        // Step 1: Generate Server RSA Key Pair
        mbedtls_pk_setup(&serverRsaKey, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
        mbedtls_rsa_context *rsa = mbedtls_pk_rsa(serverRsaKey);

        if (mbedtls_rsa_gen_key(rsa, mbedtls_ctr_drbg_random, &ctrDrbg, RSA_KEY_SIZE, 65537) != 0)

        {
            indicateSessionFailed();
            return false;
        }

        // Step 2: Receive Client Public Key
        uint8_t clientPublicKey[RSA_KEY_SIZE / 8];
        size_t clientPublicKeyLen = sizeof(clientPublicKey);

        if (!comm.receiveMessage(clientPublicKey, &clientPublicKeyLen))
        {
            indicateSessionFailed();
            return false;
        }

        // Step 3: Receive Client Public Key HMAC
        uint8_t receivedClientHmac[HMAC_SIZE];
        size_t receivedClientHmacLen = sizeof(receivedClientHmac);

        if (!comm.receiveMessage(receivedClientHmac, &receivedClientHmacLen))
        {
            indicateSessionFailed();
            return false;
        }
        // Step 4: Verify Client Public Key HMAC
        if (!verifyHMAC(clientPublicKey, clientPublicKeyLen, receivedClientHmac))
        {
            indicateSessionFailed();
            return false;
        }

        // Step 5: Import Client Public Key
        if (mbedtls_pk_parse_public_key(&clientRsaKey, clientPublicKey, clientPublicKeyLen) != 0)
        {
            indicateSessionFailed();
            return false;
        }

        // Step 6: Prepare Server Public Key
        uint8_t serverPublicKey[RSA_KEY_SIZE / 8];
        size_t serverPublicKeyLen = 0;

        // Export server public key
        if (mbedtls_pk_write_pubkey_der(&serverRsaKey, serverPublicKey, sizeof(serverPublicKey)) <= 0)
            {
                indicateSessionFailed();
                return false;
            }

        // Step 7: Encrypt Server Public Key with Client's Public Key
        uint8_t encryptedServerPubKey[RSA_KEY_SIZE / 8];
        size_t encryptedServerPubKeyLen = 0;
        
        if (mbedtls_pk_encrypt(&clientRsaKey, 
            serverPublicKey, serverPublicKeyLen,
            encryptedServerPubKey, &encryptedServerPubKeyLen,
            sizeof(encryptedServerPubKey),
            mbedtls_ctr_drbg_random, &ctrDrbg) != 0) {
            indicateSessionFailed();
            return false;
        }

        // Step 8: Send Encrypted Server Public Key
        if (!comm.sendMessage(encryptedServerPubKey, encryptedServerPubKeyLen)) {
            indicateSessionFailed();
            return false;
        }

        // Step 9: Generate and Send HMAC for Server Public Key
        uint8_t serverPubKeyHmac[HMAC_SIZE];
        if (!generateHMAC(encryptedServerPubKey, encryptedServerPubKeyLen, serverPubKeyHmac)) {
            indicateSessionFailed();
            return false;
        }
        
        if (!comm.sendMessage(serverPubKeyHmac, sizeof(serverPubKeyHmac))) {
            indicateSessionFailed();
            return false;
        }

        // Step 10: Generate AES Key and IV
        if (!generateRandomBytes(aesKey, AES_KEY_SIZE)) {
            indicateSessionFailed();
            return false;
        }
        
        if (!generateRandomBytes(sessionIV, AES_IV_SIZE)) {
            indicateSessionFailed();
            return false;
        }

        // Step 11: Prepare Final Message (New Client Public Key + Signed Secret)
        uint8_t finalMessage[RSA_KEY_SIZE / 8];
        size_t finalMessageLen = 0;

        // Combine AES Key, IV, and generate new client key pair
        mbedtls_pk_context newClientRsaKey;
        mbedtls_pk_init(&newClientRsaKey);
        
        if (mbedtls_pk_setup(&newClientRsaKey, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0) {
            indicateSessionFailed();
            return false;
        }

        mbedtls_rsa_context* newRsa = mbedtls_pk_rsa(newClientRsaKey);
        if (mbedtls_rsa_gen_key(newRsa, mbedtls_ctr_drbg_random, &ctrDrbg, RSA_KEY_SIZE, 65537) != 0) {
            indicateSessionFailed();
            return false;
        }

        // Prepare final message with new client public key, AES key, and IV
        uint8_t newClientPublicKey[RSA_KEY_SIZE / 8];
        size_t newClientPublicKeyLen = 0;

        // Similar change for newClientRsaKey

            if (mbedtls_pk_write_pubkey_der(&newClientRsaKey, newClientPublicKey, sizeof(newClientPublicKey)) <= 0)
            {
                indicateSessionFailed();
                return false;
            }

            // Combine new client public key with AES key and IV
            memcpy(finalMessage, newClientPublicKey, newClientPublicKeyLen);
            memcpy(finalMessage + newClientPublicKeyLen, aesKey, AES_KEY_SIZE);
            memcpy(finalMessage + newClientPublicKeyLen + AES_KEY_SIZE, sessionIV, AES_IV_SIZE);
            finalMessageLen = newClientPublicKeyLen + AES_KEY_SIZE + AES_IV_SIZE;

            // Encrypt final message with server's public key
            uint8_t encryptedFinalMessage[RSA_KEY_SIZE / 8];
            size_t encryptedFinalMessageLen = 0;

            if (mbedtls_pk_encrypt(&serverRsaKey,
                                   finalMessage, finalMessageLen,
                                   encryptedFinalMessage, &encryptedFinalMessageLen,
                                   sizeof(encryptedFinalMessage),
                                   mbedtls_ctr_drbg_random, &ctrDrbg) != 0)
            {
                indicateSessionFailed();
                return false;
            }

            // Send encrypted final message
            if (!comm.sendMessage(encryptedFinalMessage, encryptedFinalMessageLen))
            {
                indicateSessionFailed();
                return false;
            }

            // Generate and send HMAC
            uint8_t finalMessageHmac[HMAC_SIZE];
            if (!generateHMAC(encryptedFinalMessage, encryptedFinalMessageLen, finalMessageHmac))
            {
                indicateSessionFailed();
                return false;
            }

            if (!comm.sendMessage(finalMessageHmac, sizeof(finalMessageHmac)))
            {
                indicateSessionFailed();
                return false;
            }

            // Near the end of the method, for server confirmation
            uint8_t serverConfirmation[RSA_KEY_SIZE / 8];
            size_t serverConfirmationLen = sizeof(serverConfirmation);

            if (!comm.receiveMessage(serverConfirmation, &serverConfirmationLen))

            {
                indicateSessionFailed();
                return false;
            }

            uint8_t serverConfirmationHmac[HMAC_SIZE];
            size_t serverConfirmationHmacLen = sizeof(serverConfirmationHmac);

            if (!comm.receiveMessage(serverConfirmationHmac, &serverConfirmationHmacLen))
            {
                indicateSessionFailed();
                return false;
            }

            // Verify server confirmation HMAC
            if (!verifyHMAC(serverConfirmation, serverConfirmationLen, serverConfirmationHmac))
            {
                indicateSessionFailed();
                return false;
            }

            // Session established successfully
            sessionEstablished = true;
            lastActivityTime = millis();

            // Indicate successful session establishment
            indicateSessionEstablished();

            return true;
    }
    catch (const std::exception& e) {
        // Handle any unexpected errors
        indicateSessionFailed();
        return false;
    }
}

// LED Indication Methods
void Session::indicateSessionEstablished() {
    // Solid on for 2 seconds
    digitalWrite(LED_PIN, HIGH);
    delay(2000);
    digitalWrite(LED_PIN, LOW);
}

void Session::indicateSessionFailed() {
    // Quick blink pattern
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}

// Helper method to generate random bytes
bool Session::generateRandomBytes(uint8_t* buffer, size_t length) {
    return mbedtls_ctr_drbg_random(&ctrDrbg, buffer, length) == 0;
}

bool Session::generateHMAC(const uint8_t *data, size_t dataLen, uint8_t *hmac)
{
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0)
    {
        mbedtls_md_free(&md_ctx);
        return false;
    }

    if (mbedtls_md_hmac_starts(&md_ctx,
                               reinterpret_cast<const uint8_t *>(HMAC_SECRET_KEY),
                               strlen(HMAC_SECRET_KEY)) != 0)
    {
        mbedtls_md_free(&md_ctx);
        return false;
    }

    if (mbedtls_md_hmac_update(&md_ctx, data, dataLen) != 0)
    {
        mbedtls_md_free(&md_ctx);
        return false;
    }

    if (mbedtls_md_hmac_finish(&md_ctx, hmac) != 0)
    {
        mbedtls_md_free(&md_ctx);
        return false;
    }

    mbedtls_md_free(&md_ctx);
    return true;
}

bool Session::verifyHMAC(const uint8_t *data, size_t dataLen, const uint8_t *receivedHmac)
{
    uint8_t calculatedHmac[HMAC_SIZE];

    if (!generateHMAC(data, dataLen, calculatedHmac))
    {
        return false;
    }
    // Use constant-time comparison
    return constantTimeMemCmp(calculatedHmac, receivedHmac, HMAC_SIZE);
}

// Implement constant-time memory comparison
bool Session::constantTimeMemCmp(const uint8_t *a, const uint8_t *b, size_t length)
{
    uint8_t result = 0;
    for (size_t i = 0; i < length; i++)
    {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}

bool Session::isSessionEstablished() const
{
    // Check if session is established and not timed out
    return sessionEstablished &&
           (millis() - lastActivityTime < SESSION_TIMEOUT_MS);
}

void Session::endSession()
{
    // Clear sensitive data
    memset(aesKey, 0, sizeof(aesKey));
    memset(sessionIV, 0, sizeof(sessionIV));
    memset(sessionId, 0, sizeof(sessionId));

    // Reset session state
    sessionEstablished = false;
    lastActivityTime = 0;

    // Turn off LED
    digitalWrite(LED_PIN, LOW);

    // Reinitialize crypto contexts
    cleanupCryptoContexts();
    initializeCryptoContexts();
}

bool Session::checkSessionTimeout()
{
    if (!isSessionEstablished())
    {
        endSession();
        return false;
    }
    return true;
}

void Session::updateLastActivityTime()
{
    lastActivityTime = millis();
}

bool Session::encryptMessage(const uint8_t *input, size_t inputLen,
                             uint8_t *output, size_t &outputLen)
{
    if (!isSessionEstablished())
    {
        return false;
    }

    // Prepare AES context
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set up encryption key
    if (mbedtls_aes_setkey_enc(&aes, aesKey, AES_KEY_SIZE * 8) != 0)
    {
        mbedtls_aes_free(&aes);
        return false;
    }

    // Perform PKCS7 padding
    size_t paddedLen = inputLen + (AES_IV_SIZE - (inputLen % AES_IV_SIZE));
    uint8_t paddedInput[paddedLen];
    memcpy(paddedInput, input, inputLen);

    uint8_t paddingValue = paddedLen - inputLen;
    for (size_t i = inputLen; i < paddedLen; i++)
    {
        paddedInput[i] = paddingValue;
    }

    // Encrypt using CBC mode
    uint8_t iv[AES_IV_SIZE];
    memcpy(iv, sessionIV, AES_IV_SIZE);

    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLen,
                              iv, paddedInput, output) != 0)
    {
        mbedtls_aes_free(&aes);
        return false;
    }

    outputLen = paddedLen;
    mbedtls_aes_free(&aes);

    // Update last activity time
    updateLastActivityTime();
    return true;
}

bool Session::decryptMessage(const uint8_t *input, size_t inputLen,
                             uint8_t *output, size_t &outputLen)
{
    if (!isSessionEstablished())
    {
        return false;
    }

    // Prepare AES context
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set up decryption key
    if (mbedtls_aes_setkey_dec(&aes, aesKey, AES_KEY_SIZE * 8) != 0)
    {
        mbedtls_aes_free(&aes);
        return false;
    }

    // Prepare IV
    uint8_t iv[AES_IV_SIZE];
    memcpy(iv, sessionIV, AES_IV_SIZE);

    // Decrypt using CBC mode
    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, inputLen,
                              iv, input, output) != 0)
    {
        mbedtls_aes_free(&aes);
        return false;
    }

    // Remove PKCS7 padding
    uint8_t paddingLength = output[inputLen - 1];
    outputLen = inputLen - paddingLength;

    mbedtls_aes_free(&aes);

    // Update last activity time
    updateLastActivityTime();
    return true;
}

// Static method to calculate hash of secret key
bool Session::calculateSecretKeyHash(uint8_t *hash)
{
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);

    mbedtls_sha256_starts(&sha256_ctx, 0); // 0 for SHA-256
    mbedtls_sha256_update(&sha256_ctx,
                          reinterpret_cast<const uint8_t *>(HMAC_SECRET_KEY),
                          strlen(HMAC_SECRET_KEY));
    mbedtls_sha256_finish(&sha256_ctx, hash);

    mbedtls_sha256_free(&sha256_ctx);
    return true;
}

float Session::getCoreTemperature()
{
    return temperatureRead();
}

bool Session::toggleRelay()
{
    static bool relayState = false;
    relayState = !relayState;
    digitalWrite(32, relayState ? HIGH : LOW);
    return relayState;
}