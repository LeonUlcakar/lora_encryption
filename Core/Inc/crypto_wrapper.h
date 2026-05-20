#ifndef CRYPTO_WRAPPER_H
#define CRYPTO_WRAPPER_H

#include <stdint.h>
#include <stddef.h>

uint8_t secure_payload_encrypt(uint8_t *plaintext, size_t pt_len, uint8_t *out_buffer, size_t *out_len);
uint8_t secure_payload_decrypt(uint8_t *in_buffer, size_t in_len, uint8_t *plaintext, size_t *pt_len);

#endif
