/*
 * sx1272.h
 *
 *  Created on: Aug 5, 2025
 *      Author: Leon
 */

#ifndef INC_SX1272_H_
#define INC_SX1272_H_

#include "stm32g4xx_hal.h"
#include <stdint.h>

// Pin connections
#define SX1272_NSS_PORT      GPIOA
#define SX1272_NSS_PIN       GPIO_PIN_4
#define SX1272_RESET_PORT    GPIOA
#define SX1272_RESET_PIN     GPIO_PIN_3

#define SX1272_DIO0_PORT     GPIOB
#define SX1272_DIO0_PIN      GPIO_PIN_0

// SX1272 registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT      0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1A
#define REG_MODEM_CONFIG1        0x1D
#define REG_MODEM_CONFIG2        0x1E
#define REG_PAYLOAD_LENGTH       0x22
#define REG_DIO_MAPPING1         0x40
#define REG_IRQ_FLAGS_MASK       0x11

// IRQ Masks
#define IRQ_TX_DONE_MASK   0x08
#define IRQ_RX_DONE_MASK   0x40
#define IRQ_CRC_ERROR_MASK 0x20

// LoRa modes
#define SX1272_MODE_LORA   0x80
#define SX1272_MODE_SLEEP  0x00
#define SX1272_MODE_STDBY  0x01
#define SX1272_MODE_TX     0x03
#define SX1272_MODE_RX_CONT 0x05

// Frequency registers (add to existing register definitions)
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08

extern SPI_HandleTypeDef hspi1;

extern uint8_t SX1272_RxBuffer[256];
extern volatile uint8_t SX1272_RxLength;

void SX1272_Reset(void);
void SX1272_WriteReg(uint8_t addr, uint8_t data);
uint8_t SX1272_ReadReg(uint8_t addr);
void SX1272_WriteBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);
void SX1272_ReadBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);

void SX1272_Init(void);
void SX1272_SetupLora(void);
void SX1272_Transmit(uint8_t *data, uint8_t size);
void SX1272_Receive(void);
void SX1272_HandleDIO0(void);

#endif /* INC_SX1272_H_ */
