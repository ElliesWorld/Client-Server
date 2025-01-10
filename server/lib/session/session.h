#ifndef SESSION_H
#define SESSION_H

#include <Arduino.h>
#include <mbedtls/pk.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>

class Session
{
public:
    // Constants
    static const int RSA_KEY_SIZE = 2048;
    static const int AES_KEY_SIZE = 32;
    static const int AES_IV_SIZE = 16;
    static const int HMAC_SIZE = 32;
    static const unsigned long SESSION_TIMEOUT_MS = 60000; // 1 minute

    // Constructor and Destructor
    Session();
    ~Session();

    // Key Exchange and Session Establishment
    bool establishKeyExchange();
    bool isSessionEstablished() const;
    void endSession();

    // Cryptographic Operations
    bool encryptMessage(const uint8_t *input, size_t inputLen,
                        uint8_t *output, size_t &outputLen);
    bool decryptMessage(const uint8_t *input, size_t inputLen,
                        uint8_t *output, size_t &outputLen);

    // Session Management
    bool checkSessionTimeout();
    void updateLastActivityTime();

    // Utility Methods
    static bool calculateSecretKeyHash(uint8_t *hash);

private:
    // Cryptographic Contexts
    mbedtls_pk_context serverRsaKey;
    mbedtls_pk_context clientRsaKey;
    mbedtls_aes_context aesContext;
    mbedtls_ctr_drbg_context ctrDrbg;
    mbedtls_entropy_context entropy;

    // Session Parameters
    uint8_t aesKey[AES_KEY_SIZE];   // AES-256 key
    uint8_t sessionIV[AES_IV_SIZE]; // Initialization Vector
    uint8_t sessionId[16];          // Unique session identifier

    bool sessionEstablished;
    unsigned long lastActivityTime;

    // Hardcoded HMAC Secret Key
    static const char *HMAC_SECRET_KEY;

    // Internal Cryptographic Methods
    bool generateHMAC(const uint8_t *data, size_t dataLen, uint8_t *hmac);
    bool verifyHMAC(const uint8_t *data, size_t dataLen, const uint8_t *receivedHmac);

    // Private Helper Methods
    void initializeCryptoContexts();
    void cleanupCryptoContexts();
    bool generateRandomBytes(uint8_t *buffer, size_t length);

    // Disable copy constructor and assignment operator
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;

    // LED Control (optional, can be moved to separate class)
    void indicateSessionEstablished();
    void indicateSessionFailed();
};

#endif // SESSION_H