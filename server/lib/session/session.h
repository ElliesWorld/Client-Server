#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>
#include <stddef.h>
#include "communication.h"
#include <mbedtls/pk.h>
#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

// Define command types and block size
#define AES_BLOCK_SIZE 16 // Standard AES block size

enum CommandType
{
    SESSION_GET_TEMP = 0x01,
    SESSION_TOGGLE_LED = 0x02,
    SESSION_CLOSE = 0x03
};

class Session
{
public:
    static const int RSA_SIZE = 256;
    static const int AES_KEY_SIZE = 32;
    static const int AES_IV_SIZE = 16;
    static const uint8_t SECRET_KEY[];

    Session(const char *port, int baudrate = 115200);
    ~Session();

    bool key_exchange();
    void send_command(const uint8_t *command, size_t length);
    size_t receive_response(uint8_t *buffer, size_t length);
    void close_session();

    bool is_session_active() { return session_active; }

private:
    Communication comm;
    mbedtls_pk_context client_public_rsa;
    mbedtls_pk_context server_public_rsa;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    uint8_t aes_key[AES_KEY_SIZE];
    uint8_t iv[AES_IV_SIZE];
    bool session_active;

    void initialize_rsa();
    uint8_t *_pad(const uint8_t *data, size_t &length);
    uint8_t *_unpad(uint8_t *data, size_t &length);
};

#endif // SESSION_H