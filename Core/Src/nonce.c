#include "nonce.h"

/**
 * @brief Static counter for generating unique nonces.
 */
static uint32_t nonce_counter = 0;

/**
 * @brief Last received nonce for validation.
 */
static uint32_t last_received_nonce = 0;

/**
 * @brief Initializes the nonce system.
 * 
 * Resets the nonce counter and last received nonce to zero.
 */
void nonce_init(void) {
    nonce_counter = 0;
    last_received_nonce = 0;
}

/**
 * @brief Generates and returns the next nonce value for transmission.
 * 
 * This function increments a static counter to ensure each nonce is unique.
 * 
 * @return uint32_t The next nonce value.
 */
uint32_t nonce_generate(void) {
    return nonce_counter++;
}

/**
 * @brief Validates a received nonce.
 * 
 * Checks if the nonce is greater than the last received one to prevent replay attacks.
 * Updates the last received nonce if valid.
 * 
 * @param received_nonce The nonce received in a packet.
 * @return bool True if the nonce is valid, false otherwise.
 */
bool nonce_validate(uint32_t received_nonce) {
    if (received_nonce > last_received_nonce) {
        last_received_nonce = received_nonce;
        return true;
    }
    return false;
}

/*
 * Example usage in main loop (e.g., in main.c or sx1272.c):
 * 
 * // Initialize once at startup
 * nonce_init();
 * 
 * // When sending a packet:
 * uint32_t nonce = nonce_generate();
 * // Include nonce in packet (e.g., prepend to data)
 * // Transmit packet
 * 
 * // When receiving a packet:
 * uint32_t received_nonce = // extract from packet
 * if (nonce_validate(received_nonce)) {
 *     // Process valid packet
 * } else {
 *     // Discard invalid/replayed packet
 * }
 */