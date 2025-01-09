#include "session.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>

SecureSession::SecureSession() : session_active(false)
{
    mbedtls_pk_init(&rsa_keys);
    generate_rsa_keys();
}

SecureSession::~SecureSession()
{
    mbedtls_pk_free(&rsa_keys);
}

void SecureSession::generate_rsa_keys()
{
    // Generate RSA key pair
    if (mbedtls_pk_gen_key(&rsa_keys, MBEDTLS_PK_RSA, 2048, 65537, nullptr, nullptr) != 0)
    {
        std::cerr << "Failed to generate RSA keys." << std::endl;
    }
}

bool SecureSession::establish_session(const unsigned char *client_pubkey, size_t client_pubkey_len)
{
    // Here you would implement the logic to establish a session
    // For example, decrypt the client's public key and generate a shared secret
    // This is a placeholder for the actual implementation
    session_active = true;
    return true;
}

void SecureSession::end_session()
{
    session_active = false;
}

bool SecureSession::is_active() const
{
    return session_active;
}

std::vector<unsigned char> SecureSession::encrypt(const unsigned char *data, size_t length)
{
    if (!session_active)
    {
        throw std::runtime_error("Session is not active.");
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, aes_key, 256); // Set AES key for encryption

    std::vector<unsigned char> output(length + 16); // Output buffer (with padding)
    size_t output_length = 0;

    // Encrypt the data
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, length, aes_iv, data, output.data());

    mbedtls_aes_free(&aes);
    return output;
}

std::vector<unsigned char> SecureSession::decrypt(const unsigned char *data, size_t length)
{
    if (!session_active)
    {
        throw std::runtime_error("Session is not active.");
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, aes_key, 256); // Set AES key for decryption

    std::vector<unsigned char> output(length); // Output buffer
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, length, aes_iv, data, output.data());

    mbedtls_aes_free(&aes);
    return output;
}

unsigned char *SecureSession::get_public_key(size_t *len)
{
    return mbedtls_pk_write_pubkey(&rsa_keys, nullptr, len);
}

unsigned char *SecureSession::generate_hmac(const unsigned char *data, size_t length)
{
    unsigned char *hmac = new unsigned char[32]; // SHA256 HMAC size
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, HMAC_SECRET_KEY, strlen((const char *)HMAC_SECRET_KEY)); // Use the shared HMAC secret key
    mbedtls_md_update(&ctx, data, length);
    mbedtls_md_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);
    return hmac;
}