#include "test.h"
#include <stdio.h>
#include <string.h>

#define SYNC_MAGIC_NUM 0xFFFFFFFF

typedef enum {
    STATE_STOPPED,
    STATE_SYNCING,
    STATE_SYNC_WAIT,
    STATE_RUNNING,
    STATE_DONE
} TestState_t;

static const uint8_t default_payload_data[] = "STANDARD_TEST_PACKET_DATA_NOORDUNG_LABS";

typedef struct __attribute__((packed)) {
    uint32_t seq_num;
    uint32_t tx_time_us;
    uint8_t  payload_data[MAX_CUSTOM_PAYLOAD_SIZE];
} TestPayload_t;

static UART_HandleTypeDef *uart_handle;
static TIM_HandleTypeDef *timer_handle;
static SX1272_t *lora_tx;
static SX1272_t *lora_rx;
static uint8_t current_role;

static TestState_t current_state = STATE_STOPPED;
static uint32_t state_timer = 0;
static uint32_t test_start_time = 0;
static uint32_t last_tx_time = 0;
static uint32_t last_rx_check = 0;

static uint32_t seq_counter = 0;
static uint32_t expected_seq = 0;

static uint32_t pkts_sent = 0;
static uint32_t pkts_received = 0;
static uint32_t pkts_lost = 0;

static uint32_t total_delay_us = 0;
static uint32_t total_jitter_us = 0;
static uint32_t last_delay_us = 0;
static uint32_t max_delay_us = 0;
static uint32_t max_jitter_us = 0;

static TestPayload_t tx_pkt = {0};
static uint8_t current_tx_size = 255;

static void ResetCounters(void) {
    seq_counter = 0;
    expected_seq = 0;
    pkts_sent = 0;
    pkts_received = 0;
    pkts_lost = 0;
    total_delay_us = 0;
    total_jitter_us = 0;
    last_delay_us = 0;
    max_delay_us = 0;
    max_jitter_us = 0;
}

void Test_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim, SX1272_t *tx_mod, SX1272_t *rx_mod, uint8_t role) {
    uart_handle = huart;
    timer_handle = htim;
    lora_tx = tx_mod;
    lora_rx = rx_mod;
    current_role = role;

    Test_SetCustomPayload(NULL, 0);
}

void Test_SetCustomPayload(uint8_t *data, uint8_t size) {
    memset(tx_pkt.payload_data, 0, MAX_CUSTOM_PAYLOAD_SIZE);

    if (data != NULL && size > 0) {
        if (size > MAX_CUSTOM_PAYLOAD_SIZE) {
            size = MAX_CUSTOM_PAYLOAD_SIZE;
        }
        memcpy(tx_pkt.payload_data, data, size);
        current_tx_size = size + 8;
    } else {
        memcpy(tx_pkt.payload_data, default_payload_data, sizeof(default_payload_data));
        current_tx_size = sizeof(TestPayload_t);
    }
}

void Test_Start(void) {
    current_state = STATE_SYNCING;
    last_tx_time = 0;

    if (current_role == TEST_ROLE_MASTER) {
        char msg[] = "Initiating SYNC with remote device...\r\n";
        HAL_UART_Transmit(uart_handle, (uint8_t*)msg, strlen(msg), 100);
    } else {
        char msg[] = "Waiting for SYNC from master...\r\n";
        HAL_UART_Transmit(uart_handle, (uint8_t*)msg, strlen(msg), 100);
    }
}

static void PrintResults(void) {
    char msg[256];

    if (current_role == TEST_ROLE_MASTER) {
        uint32_t avg_delay = pkts_received > 0 ? (total_delay_us / pkts_received) : 0;
        uint32_t avg_jitter = pkts_received > 1 ? (total_jitter_us / (pkts_received - 1)) : 0;
        uint32_t loss_pct = pkts_sent > 0 ? ((pkts_sent - pkts_received) * 100 / pkts_sent) : 0;

        sprintf(msg, "\r\n--- MASTER TEST RESULTS (60s) ---\r\n"
                     "Packets Sent: %lu\r\n"
                     "Packets Rcvd: %lu\r\n"
                     "Packet Loss:  %lu%%\r\n"
                     "Avg Delay:    %lu us (Max: %lu us)\r\n"
                     "Avg Jitter:   %lu us (Max: %lu us)\r\n"
                     "---------------------------------\r\n",
                pkts_sent, pkts_received, loss_pct, avg_delay, max_delay_us, avg_jitter, max_jitter_us);
    } else {
        uint32_t rate = pkts_received / (TEST_DURATION_MS / 1000);

        sprintf(msg, "\r\n--- SLAVE TEST RESULTS (60s) ---\r\n"
                     "Packets Rcvd: %lu\r\n"
                     "Packets Lost: %lu\r\n"
                     "Avg Rx Rate:  %lu pkt/s\r\n"
                     "--------------------------------\r\n",
                pkts_received, pkts_lost, rate);
    }

    HAL_UART_Transmit(uart_handle, (uint8_t*)msg, strlen(msg), 1000);


}

void Test_Process(void) {
    uint32_t current_time = HAL_GetTick();

    switch (current_state) {
        case STATE_STOPPED:
        case STATE_DONE:
            return;

        case STATE_SYNCING:
            if (current_role == TEST_ROLE_MASTER) {
                // Keep sending sync packets until we get an echo
                if (current_time - last_tx_time >= TEST_TX_INTERVAL_MS) {
                    tx_pkt.seq_num = SYNC_MAGIC_NUM;
                    tx_pkt.tx_time_us = 0;
                    SX1272_Transmit(lora_tx, (uint8_t*)&tx_pkt, current_tx_size);
                    last_tx_time = current_time;
                }
            }
            break;

        case STATE_SYNC_WAIT:
            if (current_time - state_timer >= 100) {
                current_state = STATE_RUNNING;
                test_start_time = current_time;
                last_tx_time = 0;
                last_rx_check = current_time;

                ResetCounters();

                if (current_role == TEST_ROLE_MASTER) {
                    char msg[] = "Sync complete. Starting 60s test...\r\n";
                    HAL_UART_Transmit(uart_handle, (uint8_t*)msg, strlen(msg), 100);
                }
            }
            break;

        case STATE_RUNNING:
            if (current_time - test_start_time >= TEST_DURATION_MS) {
                current_state = STATE_DONE;
                PrintResults();
                return;
            }

            if (current_role == TEST_ROLE_MASTER && pkts_received > 0 && (current_time - last_rx_check > 2000)) {
                char msg[] = "WARNING: No response from remote device!\r\n";
                HAL_UART_Transmit(uart_handle, (uint8_t*)msg, strlen(msg), 100);
                last_rx_check = current_time;
            }

            if (current_role == TEST_ROLE_MASTER) {
                if (current_time - last_tx_time >= TEST_TX_INTERVAL_MS) {
                    tx_pkt.seq_num = seq_counter++;
                    tx_pkt.tx_time_us = __HAL_TIM_GET_COUNTER(timer_handle);

                    SX1272_Transmit(lora_tx, (uint8_t*)&tx_pkt, current_tx_size);

                    pkts_sent++;
                    last_tx_time = current_time;
                }
            }
            break;
    }
}

void Test_HandleReceive(void) {
    TestPayload_t *rx_pkt = (TestPayload_t*)lora_rx->rxBuffer;

    // Catch sync packets immediately regardless of state
    if (rx_pkt->seq_num == SYNC_MAGIC_NUM) {
        if (current_role == TEST_ROLE_MASTER && current_state == STATE_SYNCING) {
            current_state = STATE_SYNC_WAIT;
            state_timer = HAL_GetTick();
        } else if (current_role == TEST_ROLE_SLAVE) {
            SX1272_Transmit(lora_tx, (uint8_t*)rx_pkt, lora_rx->rxLength);
            current_state = STATE_SYNC_WAIT;
            state_timer = HAL_GetTick();
        }
        return;
    }

    if (current_state != STATE_RUNNING) return;

    last_rx_check = HAL_GetTick();

    if (current_role == TEST_ROLE_MASTER) {
        if (rx_pkt->seq_num >= seq_counter) return;

        uint32_t current_tick_us = __HAL_TIM_GET_COUNTER(timer_handle);
        if (current_tick_us < rx_pkt->tx_time_us) return;

        pkts_received++;

        uint32_t rtt_us = current_tick_us - rx_pkt->tx_time_us;
        uint32_t current_delay_us = rtt_us / 2;

        total_delay_us += current_delay_us;
        if (current_delay_us > max_delay_us) max_delay_us = current_delay_us;

        if (pkts_received > 1) {
            int32_t jitter = current_delay_us - last_delay_us;
            if (jitter < 0) jitter = -jitter;

            total_jitter_us += jitter;
            if ((uint32_t)jitter > max_jitter_us) max_jitter_us = jitter;
        }

        last_delay_us = current_delay_us;

    } else if (current_role == TEST_ROLE_SLAVE) {
        if (rx_pkt->seq_num > expected_seq) {
            pkts_lost += (rx_pkt->seq_num - expected_seq);
        }

        expected_seq = rx_pkt->seq_num + 1;
        pkts_received++;

        SX1272_Transmit(lora_tx, (uint8_t*)rx_pkt, lora_rx->rxLength);
    }
}
