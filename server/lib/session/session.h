#ifndef SESSION_H
#define SESSION_H

#include <mbedtls/pk.h>
#include <mbedtls/cipher.h>
#include <mbedtls/hmac.h>
#include <mbedtls/error.h>
#include <iostream>
#include <vector>
#include <string>

class SecureSession
{
private:
    mbedtls_pk_context rsa_keys; // RSA key pair
    unsigned char aes_key[32];   // AES key
    unsigned char aes_iv[16];    // AES IV
    bool session_active;

public:
    SecureSession();
    ~SecureSession();

    bool establish_session(const unsigned char *client_pubkey, size_t client_pubkey_len);
    void end_session();
    bool is_active() const;

    std::vector<unsigned char> encrypt(const unsigned char *data, size_t length);
    std::vector<unsigned char> decrypt(const unsigned char *data, size_t length);
    void generate_rsa_keys();
    unsigned char *get_public_key(size_t *len);
    unsigned char *generate_hmac(const unsigned char *data, size_t length);
};

#endif // SESSION_H