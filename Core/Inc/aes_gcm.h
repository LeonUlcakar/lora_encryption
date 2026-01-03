#ifndef AES_GCM_H
#define AES_GCM_H

#include <stdint.h>
#include <stdbool.h>

// AES key size (128 bits = 16 bytes)
#define AES_KEY_SIZE 16

// GCM nonce size (96 bits = 12 bytes, standard for GCM)
#define GCM_NONCE_SIZE 12

// GCM authentication tag size (128 bits = 16 bytes)
#define GCM_TAG_SIZE 16

/**
 * @brief Initializes the AES-GCM module.
 * 
 * This sets up the cryptographic peripheral if needed.
 * Call this once at startup after HAL_Init().
 */
void aes_gcm_init(void);

/**
 * @brief Encrypts plaintext using AES-GCM.
 * 
 * @param key         128-bit AES key (16 bytes).
 * @param nonce       96-bit nonce (12 bytes), must be unique per message.
 * @param plaintext   Data to encrypt.
 * @param plaintext_len Length of plaintext.
 * @param aad         Associated data (optional, can be NULL).
 * @param aad_len     Length of AAD.
 * @param ciphertext  Output buffer for encrypted data (same size as plaintext).
 * @param tag         Output buffer for 128-bit authentication tag (16 bytes).
 * @return bool       True if encryption succeeded, false otherwise.
 */
bool aes_gcm_encrypt(const uint8_t *key, const uint8_t *nonce,
                     const uint8_t *plaintext, uint32_t plaintext_len,
                     const uint8_t *aad, uint32_t aad_len,
                     uint8_t *ciphertext, uint8_t *tag);

/**
 * @brief Decrypts and verifies ciphertext using AES-GCM.
 * 
 * @param key         128-bit AES key (16 bytes).
 * @param nonce       96-bit nonce (12 bytes), same as used for encryption.
 * @param ciphertext  Encrypted data.
 * @param ciphertext_len Length of ciphertext.
 * @param aad         Associated data (must match encryption).
 * @param aad_len     Length of AAD.
 * @param tag         Authentication tag (16 bytes).
 * @param plaintext   Output buffer for decrypted data (same size as ciphertext).
 * @return bool       True if decryption and verification succeeded, false otherwise.
 */
bool aes_gcm_decrypt(const uint8_t *key, const uint8_t *nonce,
                     const uint8_t *ciphertext, uint32_t ciphertext_len,
                     const uint8_t *aad, uint32_t aad_len,
                     const uint8_t *tag, uint8_t *plaintext);

#endif /* AES_GCM_H */