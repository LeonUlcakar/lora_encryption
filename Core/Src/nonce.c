#include "nonce.h"

/**
 * @brief Static counter for generating unique nonces.
 * 
 * This counter increments with each call to get_next_nonce(),
 * ensuring each nonce is unique within the session.
 */
static uint32_t nonce_counter = 0;

/**
 * @brief Generates and returns the next nonce value.
 * 
 * This function increments a static counter and returns its value,
 * providing a simple mechanism for unique nonces in LoRa communications.
 * 
 * @return uint32_t The next nonce value.
 */
uint32_t get_next_nonce(void) {
    return nonce_counter++;
}