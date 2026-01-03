#ifndef NONCE_H
#define NONCE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes the nonce system.
 * 
 * Resets the nonce counter and last received nonce to zero.
 */
void nonce_init(void);

/**
 * @brief Generates and returns the next nonce value for transmission.
 * 
 * This function increments a static counter to ensure each nonce is unique.
 * 
 * @return uint32_t The next nonce value.
 */
uint32_t nonce_generate(void);

/**
 * @brief Validates a received nonce.
 * 
 * Checks if the nonce is greater than the last received one to prevent replay attacks.
 * Updates the last received nonce if valid.
 * 
 * @param received_nonce The nonce received in a packet.
 * @return bool True if the nonce is valid, false otherwise.
 */
bool nonce_validate(uint32_t received_nonce);

#endif /* NONCE_H */