
#include <openssl/conf.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include "encryption.h"


/**
 * Encrypts plaintext using AES-256-CBC.
 * 
 * @param plaintext     The input message to encrypt.
 * @param plaintext_len The length of the plaintext.
 * @param key           The AES encryption key (32 bytes).
 * @param iv            The AES initialization vector (16 bytes).
 * @param ciphertext    The output buffer for the encrypted data.
 * @return              The length of the resulting ciphertext.
 */

// Encrypts the given plaintext using AES-256-CBC and returns the length of the ciphertext
int aes_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;

    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

/**
 * Decrypts ciphertext using AES-256-CBC.
 * 
 * @param ciphertext     The encrypted message to decrypt.
 * @param ciphertext_len The length of the ciphertext.
 * @param key            The AES decryption key (32 bytes).
 * @param iv             The AES initialization vector (16 bytes).
 * @param plaintext      The output buffer for the decrypted data.
 * @return               The length of the resulting plaintext, or -1 if decryption fails.
 */
// Decrypts the given ciphertext using AES-256-CBC and returns the length of the plaintext
int aes_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len;

    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) <= 0) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}