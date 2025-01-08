#include <Arduino.h>
#include <HardwareSerial.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/aes.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/sha256.h>

// AES, HMAC, and RSA initialization
const char *HMAC_SECRET_KEY = "Fj2-;wu3Ur=ARl2!Tqi6IuKM3nG]8z1+";
const int AES_KEY_SIZE = 32;
const int AES_IV_SIZE = 16;
const int RSA_KEY_SIZE = 2048;
const int HMAC_DIGEST_SIZE = 32;
constexpr int DER_SIZE = 294;
constexpr int AES_BLOCK_SIZE = 16; // is this the vector?

// Exponent
