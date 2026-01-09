/* sx1272.h */
#ifndef INC_SX1272_H_
#define INC_SX1272_H_

#include "stm32g4xx_hal.h"
#include <stdint.h>

// --- Pin connections ---
#define SX1272_NSS_PORT      GPIOA
#define SX1272_NSS_PIN       GPIO_PIN_4
#define SX1272_RESET_PORT    GPIOA
#define SX1272_RESET_PIN     GPIO_PIN_3
#define SX1272_DIO0_PORT     GPIOB
#define SX1272_DIO0_PIN      GPIO_PIN_0

// --- SX1272 registers ---
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
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

// --- IRQ Masks ---
#define IRQ_TX_DONE_MASK   0x08
#define IRQ_RX_DONE_MASK   0x40
#define IRQ_CRC_ERROR_MASK 0x20

// --- Op Modes (Status) ---
#define SX1272_MODE_SLEEP  0x00
#define SX1272_MODE_STDBY  0x01
#define SX1272_MODE_TX     0x03
#define SX1272_MODE_RX_CONT 0x05

// --- Modulation Types (NOVO) ---
// Bit 7 v REG_OP_MODE določa: 0 = FSK/OOK, 1 = LoRa
#define SX1272_MOD_FSK     0x00
#define SX1272_MOD_LORA    0x80  // Temu praviš tudi CCR (Chirp)

// --- LoRa Parametri ---
#define SX1272_BW_125      0x00
#define SX1272_BW_250      0x40
#define SX1272_BW_500      0x80

#define SX1272_CR_4_5      0x08
#define SX1272_CR_4_6      0x10
#define SX1272_CR_4_7      0x18
#define SX1272_CR_4_8      0x20

#define SX1272_SF_6        0x60
#define SX1272_SF_7        0x70
#define SX1272_SF_8        0x80
#define SX1272_SF_9        0x90
#define SX1272_SF_10       0xA0
#define SX1272_SF_11       0xB0
#define SX1272_SF_12       0xC0

extern SPI_HandleTypeDef hspi1;
extern uint8_t SX1272_RxBuffer[256];
extern volatile uint8_t SX1272_RxLength;

// Low level
void SX1272_Reset(void);
void SX1272_WriteReg(uint8_t addr, uint8_t data);
uint8_t SX1272_ReadReg(uint8_t addr);
void SX1272_WriteBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);
void SX1272_ReadBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);
void SX1272_SetFrequency(uint32_t freq);

// High level - POSODOBLJENO
// Parametri: frekvenca, modulacija (FSK/LORA), in LoRa parametri (bw, cr, sf)
void SX1272_Init(uint32_t freq, uint8_t modulation, uint8_t bw, uint8_t cr, uint8_t sf);
void SX1272_Setup(uint32_t freq, uint8_t modulation, uint8_t bw, uint8_t cr, uint8_t sf);

void SX1272_Transmit(uint8_t *data, uint8_t size);
void SX1272_Receive(void);
void SX1272_HandleDIO0(void);

#endif /* INC_SX1272_H_ */
