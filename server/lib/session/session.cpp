#include "session.h"
#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <cstring>
#include <cstdlib>

const uint8_t Session::SECRET_KEY[] = {
    0x46, 0x6A, 0x32, 0x2D, 0x3B, 0x77, 0x75, 0x33,
    0x55, 0x72, 0x3D, 0x41, 0x52, 0x6C, 0x32, 0x21,
    0x54, 0x71, 0x69, 0x36, 0x49, 0x75, 0x4B, 0x4D,
    0x33, 0x6E, 0x47, 0x5D, 0x38, 0x7A, 0x31, 0x2B};

Session::Session(const char *port, int baudrate)
    : comm(port, baudrate), session_active(false)
{
    // Initialize contexts
    mbedtls_pk_init(&client_public_rsa);
    mbedtls_pk_init(&server_public_rsa);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
}

Session::~Session()
{
    // Free mbedTLS contexts
    mbedtls_pk_free(&client_public_rsa);
    mbedtls_pk_free(&server_public_rsa);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

void Session::initialize_rsa()
{
    // Seed the random number generator
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0) != 0)
    {
        // Handle error - potentially throw an exception or set an error flag
        return;
    }

    // Setup RSA key context
    if (mbedtls_pk_setup(&client_public_rsa, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0)
    {
        // Handle error
        return;
    }

    // Generate the RSA key pair
    if (mbedtls_rsa_gen_key(mbedtls_pk_rsa(client_public_rsa),
                            mbedtls_ctr_drbg_random, &ctr_drbg,
                            RSA_SIZE * 8, 65537) != 0)
    {
        // Handle error
        return;
    }
}

bool Session::key_exchange()
{
    // Generate client's RSA key pair
    initialize_rsa();

    // Export and send client's public key
    uint8_t public_key_buffer[RSA_SIZE];
    size_t key_len = mbedtls_pk_write_pubkey_der(&client_public_rsa, public_key_buffer, sizeof(public_key_buffer));

    if (key_len == 0 || !comm.communication_send(public_key_buffer, key_len))
    {
        return false;
    }

    // Receive server's public key
    uint8_t server_public_key[RSA_SIZE];
    if (comm.communication_read(server_public_key, sizeof(server_public_key)) != sizeof(server_public_key))
    {
        return false;
    }

    // Validate and import server's public key
    if (mbedtls_pk_parse_public_key(&server_public_rsa, server_public_key, sizeof(server_public_key)) != 0)
    {
        return false;
    }

    // Generate AES key and IV
    if (mbedtls_ctr_drbg_random(&ctr_drbg, aes_key, AES_KEY_SIZE) != 0)
    {
        return false;
    }
    if (mbedtls_ctr_drbg_random(&ctr_drbg, iv, AES_IV_SIZE) != 0)
    {
        return false;
    }

    // Encrypt AES key with server's public key
    uint8_t encrypted_aes_key[RSA_SIZE];
    size_t olen;
    if (mbedtls_pk_encrypt(&server_public_rsa, aes_key, AES_KEY_SIZE,
                           encrypted_aes_key, &olen, sizeof(encrypted_aes_key),
                           mbedtls_ctr_drbg_random, &ctr_drbg) != 0)
    {
        return false;
    }

    // Send encrypted AES key and IV
    if (!comm.communication_send(encrypted_aes_key, olen) ||
        !comm.communication_send(iv, AES_IV_SIZE))
    {
        return false;
    }

    // Verify session establishment
    uint8_t response[12];
    if (comm.communication_read(response, sizeof(response)) != sizeof(response))
    {
        return false;
    }

    // Mark session as active if verification succeeds
    session_active = (memcmp(response, "SESSION_OKAY", 12) == 0);
    return session_active;
}

void Session::send_command(const uint8_t *command, size_t length)
{
    if (!session_active)
        return;

    // Encrypt command with AES
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, aes_key, AES_KEY_SIZE * 8);

    // Prepare for padding
    size_t padded_length = length + (AES_BLOCK_SIZE - (length % AES_BLOCK_SIZE));
    uint8_t padded_command[padded_length];
    memcpy(padded_command, command, length);

    // PKCS7 Padding
    uint8_t padding_value = AES_BLOCK_SIZE - (length % AES_BLOCK_SIZE);
    memset(padded_command + length, padding_value, padding_value);

    // Encrypt
    uint8_t encrypted_command[padded_length];
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_length, iv, padded_command, encrypted_command);

    // Send command
    comm.communication_send(encrypted_command, padded_length);

    // Clean up
    mbedtls_aes_free(&aes);
}

size_t Session::receive_response(uint8_t *buffer, size_t length)
{
    if (!session_active)
        return 0;

    // Read the encrypted response
    uint8_t encrypted_response[length];
    size_t bytes_read = comm.communication_read(encrypted_response, length);

    // Decrypt the response
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, aes_key, AES_KEY_SIZE * 8);

    // Decrypt
    uint8_t decrypted_response[length];
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, bytes_read, iv, encrypted_response, decrypted_response);

    // Remove padding
    size_t padding_length = decrypted_response[bytes_read - 1];
    size_t actual_length = bytes_read - padding_length;

    // Copy to output buffer
    memcpy(buffer, decrypted_response, actual_length);

    // Clean up
    mbedtls_aes_free(&aes);

    return actual_length;
}

void Session::close_session()
{
    // Zero out sensitive data
    memset(aes_key, 0, AES_KEY_SIZE);
    memset(iv, 0, AES_IV_SIZE);

    // Free and reinitialize cryptographic contexts
    mbedtls_pk_free(&client_public_rsa);
    mbedtls_pk_free(&server_public_rsa);

    // Reinitialize contexts
    mbedtls_pk_init(&client_public_rsa);
    mbedtls_pk_init(&server_public_rsa);

    // Mark session as inactive
    session_active = false;

    // Optional: Close communication channel
    comm.communication_close();
}

bool Session::is_session_active() const
{
    return session_active;
}

void Session::reset_session()
{
    // Similar to close_session, but keeps communication channel open
    memset(aes_key, 0, AES_KEY_SIZE);
    memset(iv, 0, AES_IV_SIZE);

    mbedtls_pk_free(&client_public_rsa);
    mbedtls_pk_free(&server_public_rsa);

    mbedtls_pk_init(&client_public_rsa);
    mbedtls_pk_init(&server_public_rsa);

    session_active = false;
}

bool Session::validate_command(const uint8_t *command, size_t length)
{
    // Basic command validation
    if (!session_active)
        return false;

    // Add more sophisticated validation if needed
    // For example, check command against known command types
    switch (command[0])
    {
    case SESSION_GET_TEMP:
    case SESSION_TOGGLE_LED:
    case SESSION_CLOSE:
        return true;
    default:
        return false;
    }
}