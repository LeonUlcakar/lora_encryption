#include "crypto_wrapper.h"
#include "cmox_crypto.h"
#include <string.h>

static const uint8_t aes_key[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
static uint32_t iv_counter = 0;

uint8_t secure_payload_encrypt(uint8_t *plaintext, size_t pt_len, uint8_t *out_buffer, size_t *out_len) {
    if (pt_len > 227) return 0; // Enforce max payload size to prevent LoRa FIFO overflow

    uint8_t iv[12] = {0};
    iv_counter++;
    memcpy(iv, &iv_counter, sizeof(iv_counter)); // Embed unique counter to prevent GCM nonce reuse

    size_t computed_size = 0;

    // We write the IV to the very beginning of the transmission frame
    memcpy(&out_buffer[0], iv, 12);

    // The V4 library automatically appends the 16-byte tag to the end of the ciphertext.
    // We tell it to output everything starting at index 12, right after our IV.
    cmox_cipher_retval_t status = cmox_aead_encrypt(
        CMOX_AES_GCM_ENC_ALGO,          // Correct algorithm identifier
        plaintext, pt_len,
        16,                             // Tag length
        aes_key, sizeof(aes_key),
        iv, sizeof(iv),
        NULL, 0,                        // AAD
        &out_buffer[12],                // Output buffer (will hold Ciphertext + Tag)
        &computed_size                  // Receives total size of Ciphertext + Tag
    );

    if (status != CMOX_CIPHER_SUCCESS) return 0;

    *out_len = 12 + computed_size;
    return 1;
}

uint8_t secure_payload_decrypt(uint8_t *in_buffer, size_t in_len, uint8_t *plaintext, size_t *pt_len) {
    if (in_len <= 28) return 0; // Drop frames too small to contain IV and Tag overhead

    uint8_t iv[12];
    size_t ciphertext_and_tag_len = in_len - 12;

    memcpy(iv, &in_buffer[0], 12);

    // The decryption API expects the input buffer to be strictly [Ciphertext] + [Tag].
    // Since we packed it as [IV] + [Ciphertext] + [Tag], we pass the buffer starting at index 12.
    cmox_cipher_retval_t status = cmox_aead_decrypt(
        CMOX_AES_GCM_DEC_ALGO,          // Correct algorithm identifier
        &in_buffer[12], ciphertext_and_tag_len,
        16,                             // Expected tag length
        aes_key, sizeof(aes_key),
        iv, sizeof(iv),
        NULL, 0,                        // AAD
        plaintext,                      // Output buffer for decrypted data
        pt_len                          // Receives actual length of plaintext
    );

    if (status != CMOX_CIPHER_SUCCESS) return 0; // Reject tampered or corrupted payloads

    return 1;
}

// ST Cryptographic Library Hardware Hooks
// The library expects these to exist to initialize hardware accelerators or clocks.
// Since the G431 runs in software mode, we just return success to satisfy the linker.

cmox_init_retval_t cmox_ll_init(void *pArg)
{
  (void)pArg;
  /* Ensure CRC is enabled for cryptographic processing */
  //__HAL_RCC_CRC_RELEASE_RESET();
  //__HAL_RCC_CRC_CLK_ENABLE();
  return CMOX_INIT_SUCCESS;
}

/**
  * @brief          CMOX library low level de-initialization
  * @param          pArg User defined parameter that is transmitted from finalize service
  * @retval         De-initialization status: @ref CMOX_INIT_SUCCESS / @ref CMOX_INIT_FAIL
  */
cmox_init_retval_t cmox_ll_deInit(void *pArg)
{
  (void)pArg;
  /* Do not turn off CRC to avoid side effect on other SW parts using it */
  return CMOX_INIT_SUCCESS;
}
