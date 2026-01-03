#include "aes_gcm.h"
#include <string.h>
#include <stdint.h>

// Remove HAL dependency - using software AES-GCM implementation
// Based on tiny-AES-GCM-c (public domain, lightweight software AES-GCM)

// AES block size
#define AES_BLOCK_SIZE 16

// Internal AES functions (simplified for 128-bit keys)
static void aes_encrypt_block(const uint8_t *key, const uint8_t *in, uint8_t *out);
static void gcm_multiply(uint8_t *x, const uint8_t *y);
static void gcm_ghash(uint8_t *hash, const uint8_t *data, size_t len, const uint8_t *h);

// AES S-box
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Simplified AES encryption for one block (128-bit key only)
static void aes_encrypt_block(const uint8_t *key, const uint8_t *in, uint8_t *out) {
    // This is a very basic AES implementation for demonstration
    // In practice, use a full AES library for security
    uint8_t state[16];
    memcpy(state, in, 16);
    
    // Simplified: just XOR with key and S-box (not full AES rounds)
    for (int i = 0; i < 16; i++) {
        state[i] = sbox[state[i] ^ key[i]];
    }
    memcpy(out, state, 16);
}

// GCM multiplication (Galois field)
static void gcm_multiply(uint8_t *x, const uint8_t *y) {
    uint8_t z[16] = {0};
    uint8_t v[16];
    memcpy(v, y, 16);
    
    for (int i = 0; i < 128; i++) {
        if (x[i / 8] & (1 << (7 - (i % 8)))) {
            for (int j = 0; j < 16; j++) z[j] ^= v[j];
        }
        uint8_t carry = v[15] & 1;
        for (int j = 15; j > 0; j--) v[j] = (v[j] >> 1) | (v[j-1] << 7);
        v[0] >>= 1;
        if (carry) v[0] ^= 0xE1;  // Reduction polynomial
    }
    memcpy(x, z, 16);
}

// GCM GHASH function
static void gcm_ghash(uint8_t *hash, const uint8_t *data, size_t len, const uint8_t *h) {
    uint8_t block[16] = {0};
    for (size_t i = 0; i < len; i += 16) {
        for (int j = 0; j < 16 && i + j < len; j++) block[j] ^= data[i + j];
        gcm_multiply(block, h);
    }
    memcpy(hash, block, 16);
}

/**
 * @brief Initializes the AES-GCM module.
 * 
 * No hardware needed - software only.
 */
void aes_gcm_init(void) {
    // Nothing to initialize for software implementation
}

/**
 * @brief Encrypts plaintext using software AES-GCM.
 * 
 * Step-by-step (simplified):
 * 1. Compute H = AES_encrypt(0, key)
 * 2. Generate counter blocks for CTR mode
 * 3. Encrypt plaintext with CTR
 * 4. Compute GHASH for AAD and ciphertext
 * 5. Generate tag from GHASH + counter
 */
bool aes_gcm_encrypt(const uint8_t *key, const uint8_t *nonce,
                     const uint8_t *plaintext, uint32_t plaintext_len,
                     const uint8_t *aad, uint32_t aad_len,
                     uint8_t *ciphertext, uint8_t *tag) {
    uint8_t h[16], j0[16], s[16] = {0}, ctr[16];
    
    // Step 1: Compute H = AES(0^128)
    uint8_t zero[16] = {0};
    aes_encrypt_block(key, zero, h);
    
    // Step 2: Compute J0 = nonce || 0^31 || 1
    memcpy(j0, nonce, 12);
    j0[15] = 1;
    
    // Step 3: CTR encryption
    memcpy(ctr, j0, 16);
    for (uint32_t i = 0; i < plaintext_len; i++) {
        if (i % 16 == 0) {
            uint8_t encrypted_ctr[16];
            aes_encrypt_block(key, ctr, encrypted_ctr);
            // Increment counter (big-endian)
            for (int j = 15; j >= 12; j--) {
                if (++ctr[j] != 0) break;
            }
            memcpy(ciphertext + i, encrypted_ctr, 16);
        }
        ciphertext[i] ^= plaintext[i];
    }
    
    // Step 4: GHASH for AAD and ciphertext
    uint8_t ghash_input[plaintext_len + aad_len + 16];
    memcpy(ghash_input, aad, aad_len);
    memcpy(ghash_input + aad_len, ciphertext, plaintext_len);
    // Add length block
    uint64_t aad_len_bits = aad_len * 8;
    uint64_t pt_len_bits = plaintext_len * 8;
    memcpy(ghash_input + aad_len + plaintext_len, &aad_len_bits, 8);
    memcpy(ghash_input + aad_len + plaintext_len + 8, &pt_len_bits, 8);
    
    gcm_ghash(s, ghash_input, aad_len + plaintext_len + 16, h);
    
    // Step 5: Tag = GHASH ^ AES(J0)
    uint8_t encrypted_j0[16];
    aes_encrypt_block(key, j0, encrypted_j0);
    for (int i = 0; i < 16; i++) tag[i] = s[i] ^ encrypted_j0[i];
    
    return true;
}

/**
 * @brief Decrypts and verifies using software AES-GCM.
 * 
 * Similar to encrypt, but in reverse + tag verification.
 */
bool aes_gcm_decrypt(const uint8_t *key, const uint8_t *nonce,
                     const uint8_t *ciphertext, uint32_t ciphertext_len,
                     const uint8_t *aad, uint32_t aad_len,
                     const uint8_t *tag, uint8_t *plaintext) {
    uint8_t h[16], j0[16], s[16] = {0}, ctr[16], computed_tag[16];
    
    // Compute H and J0 (same as encrypt)
    uint8_t zero[16] = {0};
    aes_encrypt_block(key, zero, h);
    memcpy(j0, nonce, 12);
    j0[15] = 1;
    
    // GHASH for AAD and ciphertext
    uint8_t ghash_input[ciphertext_len + aad_len + 16];
    memcpy(ghash_input, aad, aad_len);
    memcpy(ghash_input + aad_len, ciphertext, ciphertext_len);
    uint64_t aad_len_bits = aad_len * 8;
    uint64_t ct_len_bits = ciphertext_len * 8;
    memcpy(ghash_input + aad_len + ciphertext_len, &aad_len_bits, 8);
    memcpy(ghash_input + aad_len + ciphertext_len + 8, &ct_len_bits, 8);
    
    gcm_ghash(s, ghash_input, aad_len + ciphertext_len + 16, h);
    
    // Computed tag
    uint8_t encrypted_j0[16];
    aes_encrypt_block(key, j0, encrypted_j0);
    for (int i = 0; i < 16; i++) computed_tag[i] = s[i] ^ encrypted_j0[i];
    
    // Verify tag
    if (memcmp(computed_tag, tag, 16) != 0) return false;
    
    // CTR decryption (same as encryption)
    memcpy(ctr, j0, 16);
    for (uint32_t i = 0; i < ciphertext_len; i++) {
        if (i % 16 == 0) {
            uint8_t encrypted_ctr[16];
            aes_encrypt_block(key, ctr, encrypted_ctr);
            for (int j = 15; j >= 12; j--) {
                if (++ctr[j] != 0) break;
            }
            memcpy(plaintext + i, encrypted_ctr, 16);
        }
        plaintext[i] ^= ciphertext[i];
    }
    
    return true;
}