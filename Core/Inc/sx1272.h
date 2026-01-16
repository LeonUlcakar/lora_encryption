/* sx1272.h */
#ifndef INC_SX1272_H_
#define INC_SX1272_H_

#include "stm32g4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// --- Register Map ---
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT      0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_MODEM_CONFIG1        0x1D
#define REG_MODEM_CONFIG2        0x1E
#define REG_PAYLOAD_LENGTH       0x22
#define REG_DIO_MAPPING1         0x40

// --- Constants ---
#define SX1272_MODE_SLEEP       0x00
#define SX1272_MODE_STDBY       0x01
#define SX1272_MODE_TX          0x03
#define SX1272_MODE_RX_CONT     0x05
#define SX1272_MOD_LORA         0x80

// --- LoRa Settings ---
#define SX1272_BW_125           0x00
#define SX1272_BW_250           0x40
#define SX1272_BW_500           0x80
#define SX1272_CR_4_5           0x08
#define SX1272_SF_7             0x70
#define SX1272_SF_12            0xC0

// --- Instance Structure ---
typedef struct {
    // SPI Handle
    SPI_HandleTypeDef *hspi;

    // GPIO Control Pins
    GPIO_TypeDef *NSS_Port;   uint16_t NSS_Pin;
    GPIO_TypeDef *Reset_Port; uint16_t Reset_Pin;
    GPIO_TypeDef *DIO0_Port;  uint16_t DIO0_Pin;

    // RF Switch Pins (Optional)
    GPIO_TypeDef *TX_SW_Port; uint16_t TX_SW_Pin;
    GPIO_TypeDef *RX_SW_Port; uint16_t RX_SW_Pin;

    // Data Buffers
    uint8_t rxBuffer[256];
    uint8_t rxLength;
    volatile bool packetReceived;

} SX1272_t;

// --- Functions ---
void SX1272_Init(SX1272_t *mod, SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *nssP, uint16_t nssPin,
                 GPIO_TypeDef *rstP, uint16_t rstPin,
                 GPIO_TypeDef *dioP, uint16_t dioPin);

void SX1272_ConfigAntennaSwitch(SX1272_t *mod,
                                GPIO_TypeDef *txP, uint16_t txPin,
                                GPIO_TypeDef *rxP, uint16_t rxPin);

void SX1272_Setup(SX1272_t *mod, uint32_t freq, uint8_t bw, uint8_t cr, uint8_t sf);
void SX1272_Transmit(SX1272_t *mod, uint8_t *data, uint8_t size);
void SX1272_Receive(SX1272_t *mod);
void SX1272_HandleDIO0(SX1272_t *mod); // Call in HAL_GPIO_EXTI_Callback

// Low level access if needed
void SX1272_WriteReg(SX1272_t *mod, uint8_t addr, uint8_t data);
uint8_t SX1272_ReadReg(SX1272_t *mod, uint8_t addr);

#endif /* INC_SX1272_H_ */
