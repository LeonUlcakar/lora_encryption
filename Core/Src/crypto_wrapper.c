#include "crypto_wrapper.h"
#include "cmox_crypto.h"
#include "stm32g4xx_hal.h"
#include <string.h>

static const uint8_t aes_key[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
static uint32_t iv_counter = 0;

uint8_t secure_payload_encrypt(uint8_t *plaintext, size_t pt_len, uint8_t *out_buffer, size_t *out_len) {
    if (pt_len > 227) return 0;

    uint32_t align_pt[64] = {0};
    uint32_t align_ct[64] = {0};

    memcpy(align_pt, plaintext, pt_len);

    uint8_t iv[12] = {0};
    iv_counter++;
    memcpy(iv, &iv_counter, sizeof(iv_counter));

    size_t computed_size = 0;

    cmox_cipher_retval_t status = cmox_aead_encrypt(
        CMOX_AES_GCM_ENC_ALGO,
        (uint8_t*)align_pt, pt_len,
        16,
        aes_key, sizeof(aes_key),
        iv, sizeof(iv),
        NULL, 0,
        (uint8_t*)align_ct,
        &computed_size
    );

    if (status != CMOX_CIPHER_SUCCESS) return 0;

    memcpy(&out_buffer[0], iv, 12);
    memcpy(&out_buffer[12], align_ct, computed_size);

    *out_len = 12 + computed_size;
    return 1;
}

uint8_t secure_payload_decrypt(uint8_t *in_buffer, size_t in_len, uint8_t *plaintext, size_t *pt_len) {
    if (in_len <= 28) return 0;

    uint32_t align_in[64] = {0};
    uint32_t align_out[64] = {0};

    uint8_t iv[12];

    size_t ciphertext_len = in_len - 28;
    size_t ciphertext_and_tag_len = in_len - 12;

    memcpy(iv, &in_buffer[0], 12);
    memcpy(align_in, &in_buffer[12], ciphertext_and_tag_len);

    cmox_cipher_retval_t status = cmox_aead_decrypt(
        CMOX_AES_GCM_DEC_ALGO,
        (uint8_t*)align_in, ciphertext_len,
        16,
        aes_key, sizeof(aes_key),
        iv, sizeof(iv),
        NULL, 0,
        (uint8_t*)align_out,
        pt_len
    );

    if (status != CMOX_CIPHER_SUCCESS) return 0;

    memcpy(plaintext, align_out, *pt_len);
    return 1;
}

cmox_init_retval_t cmox_ll_init(void *pArg) {
    __HAL_RCC_CRC_CLK_ENABLE(); // Required for GHASH Galois Field multiplication
    return CMOX_INIT_SUCCESS;
}

cmox_init_retval_t cmox_ll_deInit(void *pArg) {
    __HAL_RCC_CRC_CLK_DISABLE();
    return CMOX_INIT_SUCCESS;
}
