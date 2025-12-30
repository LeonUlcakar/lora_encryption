#ifndef NONCE_H
#define NONCE_H

#include <stdint.h>

/**
 * @brief Generates and returns the next nonce value.
 * 
 * This function provides a simple counter-based nonce to ensure uniqueness
 * in communications, preventing replay attacks.
 * 
 * @return uint32_t The next nonce value.
 */
uint32_t get_next_nonce(void);

#endif /* NONCE_H */