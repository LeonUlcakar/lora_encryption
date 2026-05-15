#include "test.h"
#include <stdio.h>
#include <string.h>

static const uint8_t default_payload_data[] = "STANDARD_TEST_PACKET_DATA_NOORDUNG_LABS";

typedef struct __attribute__((packed)) {
	uint32_t seq_num;
	uint32_t tx_time_ms;
	uint8_t payload_data[MAX_CUSTOM_PAYLOAD_SIZE];
} TestPayload_t;

static UART_HandleTypeDef *uart_handle;
static SX1272_t *lora_tx;
static SX1272_t *lora_rx;
static uint8_t current_role;

static bool test_running = false;
static uint32_t test_start_time = 0;
static uint32_t last_tx_time = 0;

static uint32_t seq_counter = 0;
static uint32_t expected_seq = 0;

static uint32_t pkts_sent = 0;
static uint32_t pkts_received = 0;
static uint32_t pkts_lost = 0;

static uint32_t total_delay = 0;
static uint32_t total_jitter = 0;
static uint32_t last_delay = 0;
static uint32_t max_delay = 0;
static uint32_t max_jitter = 0;

static TestPayload_t tx_pkt = { 0 };
static uint8_t current_tx_size = 255;

void Test_Init(UART_HandleTypeDef *huart, SX1272_t *tx_mod, SX1272_t *rx_mod, uint8_t role) {
	uart_handle = huart;
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
		current_tx_size = size + 8; // Accounts for seq_num and tx_time_ms headers
	} else {
		memcpy(tx_pkt.payload_data, default_payload_data, sizeof(default_payload_data));
		current_tx_size = sizeof(TestPayload_t);
	}
}

void Test_Start(void) {
	test_running = true;
	test_start_time = HAL_GetTick();
	last_tx_time = 0;

	seq_counter = 0;
	expected_seq = 0;
	pkts_sent = 0;
	pkts_received = 0;
	pkts_lost = 0;
	total_delay = 0;
	total_jitter = 0;
	last_delay = 0;
	max_delay = 0;
	max_jitter = 0;
}

static void PrintResults(void) {
	char msg[256];

	if (current_role == TEST_ROLE_MASTER) {
		uint32_t avg_delay = pkts_received > 0 ? (total_delay / pkts_received) : 0;
		uint32_t avg_jitter = pkts_received > 1 ? (total_jitter / (pkts_received - 1)) : 0;
		uint32_t loss_pct = pkts_sent > 0 ? ((pkts_sent - pkts_received) * 100 / pkts_sent) : 0;

		sprintf(msg, "\r\n--- MASTER TEST RESULTS (60s) ---\r\n"
				"Packets Sent: %lu\r\n"
				"Packets Rcvd: %lu\r\n"
				"Packet Loss:  %lu%%\r\n"
				"Avg Delay:    %lu ms (Max: %lu ms)\r\n"
				"Avg Jitter:   %lu ms (Max: %lu ms)\r\n"
				"---------------------------------\r\n", pkts_sent, pkts_received, loss_pct, avg_delay, max_delay, avg_jitter, max_jitter);
	} else {
		uint32_t rate = pkts_received / (TEST_DURATION_MS / 1000);

		sprintf(msg, "\r\n--- SLAVE TEST RESULTS (60s) ---\r\n"
				"Packets Rcvd: %lu\r\n"
				"Packets Lost: %lu (Sequence gaps)\r\n"
				"Avg Rx Rate:  %lu pkt/s\r\n"
				"--------------------------------\r\n", pkts_received, pkts_lost, rate);
	}

	HAL_UART_Transmit(uart_handle, (uint8_t*) msg, strlen(msg), 1000);
}

void Test_Process(void) {
	if (!test_running)
		return;

	uint32_t current_time = HAL_GetTick();

	if (current_time - test_start_time >= TEST_DURATION_MS) {
		test_running = false;
		PrintResults();
		return;
	}

	if (current_role == TEST_ROLE_MASTER) {
		if (current_time - last_tx_time >= TEST_TX_INTERVAL_MS) {
			tx_pkt.seq_num = seq_counter++;
			tx_pkt.tx_time_ms = current_time;

			SX1272_Transmit(lora_tx, (uint8_t*) &tx_pkt, current_tx_size);

			pkts_sent++;
			last_tx_time = current_time;
		}
	}
}

void Test_HandleReceive(void) {
	if (!test_running)
		return;

	TestPayload_t *rx_pkt = (TestPayload_t*) lora_rx->rxBuffer;

	if (current_role == TEST_ROLE_MASTER) {
		pkts_received++;

		uint32_t current_tick = HAL_GetTick();
		uint32_t rtt = current_tick - rx_pkt->tx_time_ms;
		uint32_t current_delay = rtt / 2;

		total_delay += current_delay;
		if (current_delay > max_delay)
			max_delay = current_delay;

		if (pkts_received > 1) {
			int32_t jitter = current_delay - last_delay;
			if (jitter < 0)
				jitter = -jitter; // Absolute value conversion

			total_jitter += jitter;
			if ((uint32_t) jitter > max_jitter)
				max_jitter = jitter;
		}

		last_delay = current_delay;

	} else if (current_role == TEST_ROLE_SLAVE) {
		if (rx_pkt->seq_num > expected_seq) {
			pkts_lost += (rx_pkt->seq_num - expected_seq);
		}

		expected_seq = rx_pkt->seq_num + 1;
		pkts_received++;

		SX1272_Transmit(lora_tx, (uint8_t*) rx_pkt, lora_rx->rxLength);
	}
}
