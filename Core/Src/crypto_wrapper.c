#include "crypto_wrapper.h"
#include <string.h>

/* ========================================================================== */
/*                              AES-128 CORE                                  */
/* ========================================================================== */

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

static const uint8_t rcon[10] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

static uint8_t aes_round_keys[176]; // 11 rounds * 16 bytes

static void aes_key_expansion(const uint8_t *key) {
    memcpy(aes_round_keys, key, 16);
    uint8_t temp[4];
    for (int i = 16; i < 176; i += 4) {
        memcpy(temp, &aes_round_keys[i - 4], 4);
        if (i % 16 == 0) {
            uint8_t k = temp[0];
            temp[0] = sbox[temp[1]] ^ rcon[(i / 16) - 1];
            temp[1] = sbox[temp[2]];
            temp[2] = sbox[temp[3]];
            temp[3] = sbox[k];
        }
        aes_round_keys[i] = aes_round_keys[i - 16] ^ temp[0];
        aes_round_keys[i + 1] = aes_round_keys[i - 15] ^ temp[1];
        aes_round_keys[i + 2] = aes_round_keys[i - 14] ^ temp[2];
        aes_round_keys[i + 3] = aes_round_keys[i - 13] ^ temp[3];
    }
}

static void add_round_key(uint8_t *state, const uint8_t *round_key) {
    for (int i = 0; i < 16; ++i) {
        state[i] ^= round_key[i];
    }
}

static void sub_bytes(uint8_t *state) {
    for (int i = 0; i < 16; ++i) {
        state[i] = sbox[state[i]];
    }
}

static void shift_rows(uint8_t *state) {
    uint8_t temp;
    temp = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = temp;
    temp = state[2]; state[2] = state[10]; state[10] = temp;
    temp = state[6]; state[6] = state[14]; state[14] = temp;
    temp = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = temp;
}

static uint8_t xtime(uint8_t x) {
    return (x << 1) ^ (((x >> 7) & 1) * 0x1b);
}

static void mix_columns(uint8_t *state) {
    uint8_t tmp, tm, t;
    for (int i = 0; i < 16; i += 4) {
        t = state[i];
        tmp = state[i] ^ state[i+1] ^ state[i+2] ^ state[i+3];
        tm = state[i] ^ state[i+1]; tm = xtime(tm); state[i] ^= tm ^ tmp;
        tm = state[i+1] ^ state[i+2]; tm = xtime(tm); state[i+1] ^= tm ^ tmp;
        tm = state[i+2] ^ state[i+3]; tm = xtime(tm); state[i+2] ^= tm ^ tmp;
        tm = state[i+3] ^ t; tm = xtime(tm); state[i+3] ^= tm ^ tmp;
    }
}

static void aes_encrypt_block(const uint8_t *in, uint8_t *out) {
    uint8_t state[16];
    memcpy(state, in, 16);

    add_round_key(state, aes_round_keys);

    for (int round = 1; round < 10; ++round) {
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        add_round_key(state, aes_round_keys + (round * 16));
    }

    sub_bytes(state);
    shift_rows(state);
    add_round_key(state, aes_round_keys + 160);

    memcpy(out, state, 16);
}

/* ========================================================================== */
/*                              GCM CORE                                      */
/* ========================================================================== */

// Galois Field (2^128) Multiplication
static void gf_mult(const uint8_t x[16], const uint8_t y[16], uint8_t out[16]) {
    uint8_t z[16] = {0};
    uint8_t v[16];
    memcpy(v, y, 16);

    for (int i = 0; i < 128; i++) {
        if (x[i / 8] & (1 << (7 - (i % 8)))) {
            for (int j = 0; j < 16; j++) {
                z[j] ^= v[j];
            }
        }
        uint8_t lsb = v[15] & 1;
        for (int j = 15; j > 0; j--) {
            v[j] = (v[j] >> 1) | (v[j - 1] << 7);
        }
        v[0] >>= 1;
        if (lsb) {
            v[0] ^= 0xE1;
        }
    }
    memcpy(out, z, 16);
}

// Increment the rightmost 32 bits of the counter block
static void inc32(uint8_t block[16]) {
    for (int i = 15; i >= 12; i--) {
        if (++block[i] != 0) break;
    }
}

// GHASH Function
static void ghash(const uint8_t h[16], const uint8_t *data, size_t len, uint8_t y[16]) {
    uint8_t tmp[16];
    size_t i = 0;
    while (i < len) {
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                y[j] ^= data[i + j];
            }
        }
        gf_mult(y, h, tmp);
        memcpy(y, tmp, 16);
        i += 16;
    }
}

/* ========================================================================== */
/*                             WRAPPER API                                    */
/* ========================================================================== */

// Secret AES 128-bit key
static const uint8_t aes_key[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

static bool initialized = false;

static void ensure_init() {
    if (!initialized) {
        aes_key_expansion(aes_key);
        initialized = true;
    }
}

bool secure_payload_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len) {
    ensure_init();

    // 1. Generate Nonce/IV (12 bytes)
    static uint32_t iv_counter = 0;
    uint8_t iv[12] = {0};
    iv[0] = (iv_counter >> 24) & 0xFF;
    iv[1] = (iv_counter >> 16) & 0xFF;
    iv[2] = (iv_counter >> 8) & 0xFF;
    iv[3] = iv_counter & 0xFF;
    iv_counter++;

    // 2. Precalculate H for GCM
    uint8_t h[16] = {0};
    uint8_t zero_block[16] = {0};
    aes_encrypt_block(zero_block, h);

    // 3. Prepare J0 (Counter block 0)
    uint8_t j0[16] = {0};
    memcpy(j0, iv, 12);
    j0[15] = 0x01;

    // 4. Encrypt Data using GCTR
    uint8_t cb[16];
    memcpy(cb, j0, 16);
    inc32(cb); // Start from J0 + 1

    uint8_t e_cb[16];
    size_t offset = 0;

    // First 12 bytes of output is the IV itself
    memcpy(out, iv, 12);
    uint8_t *ciphertext = out + 12;

    while (offset < in_len) {
        aes_encrypt_block(cb, e_cb);
        size_t chunk = (in_len - offset > 16) ? 16 : (in_len - offset);
        for (size_t i = 0; i < chunk; i++) {
            ciphertext[offset + i] = in[offset + i] ^ e_cb[i];
        }
        offset += chunk;
        inc32(cb);
    }

    // 5. Calculate Authentication Tag (GHASH)
    uint8_t s[16] = {0};
    // No AAD to GHASH in this implementation, strictly ciphertext.
    ghash(h, ciphertext, in_len, s);

    // Append length block (64-bit AAD len, 64-bit Ciphertext len)
    uint8_t len_block[16] = {0};
    uint64_t c_bits = (uint64_t)in_len * 8;
    for (int i = 0; i < 8; i++) {
        len_block[15 - i] = (c_bits >> (i * 8)) & 0xFF;
    }

    uint8_t s_final[16] = {0};
    memcpy(s_final, s, 16);
    for (int i = 0; i < 16; i++) s_final[i] ^= len_block[i];

    uint8_t tag_pre[16];
    gf_mult(s_final, h, tag_pre);

    aes_encrypt_block(j0, e_cb);
    uint8_t *tag = ciphertext + in_len;
    for (int i = 0; i < 16; i++) {
        tag[i] = tag_pre[i] ^ e_cb[i];
    }

    *out_len = 12 + in_len + 16;
    return true;
}

bool secure_payload_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len) {
    if (in_len < 28) {
        return false; // Minimum length: 12 (IV) + 16 (Tag)
    }

    ensure_init();

    size_t cipher_len = in_len - 28;
    const uint8_t *iv = in;
    const uint8_t *ciphertext = in + 12;
    const uint8_t *received_tag = in + 12 + cipher_len;

    uint8_t h[16] = {0};
    uint8_t zero_block[16] = {0};
    aes_encrypt_block(zero_block, h);

    uint8_t j0[16] = {0};
    memcpy(j0, iv, 12);
    j0[15] = 0x01;

    // 1. Verify Authentication Tag first (Encrypt-then-MAC style verification)
    uint8_t s[16] = {0};
    ghash(h, ciphertext, cipher_len, s);

    uint8_t len_block[16] = {0};
    uint64_t c_bits = (uint64_t)cipher_len * 8;
    for (int i = 0; i < 8; i++) {
        len_block[15 - i] = (c_bits >> (i * 8)) & 0xFF;
    }

    uint8_t s_final[16] = {0};
    memcpy(s_final, s, 16);
    for (int i = 0; i < 16; i++) s_final[i] ^= len_block[i];

    uint8_t tag_pre[16];
    gf_mult(s_final, h, tag_pre);

    uint8_t e_cb[16];
    aes_encrypt_block(j0, e_cb);

    uint8_t calculated_tag[16];
    for (int i = 0; i < 16; i++) {
        calculated_tag[i] = tag_pre[i] ^ e_cb[i];
    }

    // Constant-time tag comparison
    uint8_t tag_diff = 0;
    for (int i = 0; i < 16; i++) {
        tag_diff |= (calculated_tag[i] ^ received_tag[i]);
    }

    if (tag_diff != 0) {
        return false; // Authentication failed
    }

    // 2. Decrypt data
    uint8_t cb[16];
    memcpy(cb, j0, 16);
    inc32(cb);

    size_t offset = 0;
    while (offset < cipher_len) {
        aes_encrypt_block(cb, e_cb);
        size_t chunk = (cipher_len - offset > 16) ? 16 : (cipher_len - offset);
        for (size_t i = 0; i < chunk; i++) {
            out[offset + i] = ciphertext[offset + i] ^ e_cb[i];
        }
        offset += chunk;
        inc32(cb);
    }

    *out_len = cipher_len;
    return true;
}
