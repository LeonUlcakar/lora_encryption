#ifndef INC_TEST_H_
#define INC_TEST_H_

#include "stm32g4xx_hal.h"
#include "sx1272.h"
#include <stdint.h>
#include <stdbool.h>

#define TEST_ROLE_MASTER 0
#define TEST_ROLE_SLAVE  1

#define TEST_DURATION_MS 60000
#define TEST_TX_INTERVAL_MS 500
#define MAX_CUSTOM_PAYLOAD_SIZE 247 // 255 - 8 bytes for timing/sequence headers

void Test_Init(UART_HandleTypeDef *huart, SX1272_t *tx_mod, SX1272_t *rx_mod, uint8_t role);
void Test_SetCustomPayload(uint8_t *data, uint8_t size);
void Test_Start(void);
void Test_Process(void);
void Test_HandleReceive(void);

#endif
