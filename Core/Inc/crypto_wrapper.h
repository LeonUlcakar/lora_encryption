#ifndef CRYPTO_WRAPPER_H
#define CRYPTO_WRAPPER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Encrypts plaintext into output buffer.
 * Output format: [12-byte IV] + [Ciphertext] + [16-byte MAC Tag]
 * out_len will be in_len + 28.
 * out buffer must be large enough! (e.g., 255 bytes for LoRa)
 */
bool secure_payload_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len);

/* Decrypts payload and verifies authentication tag.
 * Input format: [12-byte IV] + [Ciphertext] + [16-byte MAC Tag]
 * out_len will be in_len - 28.
 * Returns true if authentication succeeds and decryption is valid.
 */
bool secure_payload_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len);

#endif /* CRYPTO_WRAPPER_H */
