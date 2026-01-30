/* sx1272.c */
#include "sx1272.h"

static void SX1272_Select(SX1272_t *mod)   { HAL_GPIO_WritePin(mod->NSS_Port, mod->NSS_Pin, GPIO_PIN_RESET); }
static void SX1272_Unselect(SX1272_t *mod) { HAL_GPIO_WritePin(mod->NSS_Port, mod->NSS_Pin, GPIO_PIN_SET); }

static void SX1272_SetAntenna(SX1272_t *mod, uint8_t mode) {
    if (mod->TX_SW_Port == NULL) return;
    if (mode == SX1272_MODE_TX) {
        HAL_GPIO_WritePin(mod->TX_SW_Port, mod->TX_SW_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(mod->RX_SW_Port, mod->RX_SW_Pin, GPIO_PIN_RESET);
    } else if (mode == SX1272_MODE_RX_CONT) {
        HAL_GPIO_WritePin(mod->TX_SW_Port, mod->TX_SW_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(mod->RX_SW_Port, mod->RX_SW_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(mod->TX_SW_Port, mod->TX_SW_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(mod->RX_SW_Port, mod->RX_SW_Pin, GPIO_PIN_RESET);
    }
}

void SX1272_WriteReg(SX1272_t *mod, uint8_t addr, uint8_t data) {
    addr |= 0x80;
    SX1272_Select(mod);
    HAL_SPI_Transmit(mod->hspi, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(mod->hspi, &data, 1, HAL_MAX_DELAY);
    SX1272_Unselect(mod);
}

uint8_t SX1272_ReadReg(SX1272_t *mod, uint8_t addr) {
    uint8_t val = 0;
    addr &= 0x7F;
    SX1272_Select(mod);
    HAL_SPI_Transmit(mod->hspi, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(mod->hspi, &val, 1, HAL_MAX_DELAY);
    SX1272_Unselect(mod);
    return val;
}

void SX1272_WriteBuffer(SX1272_t *mod, uint8_t addr, uint8_t *buffer, uint8_t size) {
    addr |= 0x80;
    SX1272_Select(mod);
    HAL_SPI_Transmit(mod->hspi, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(mod->hspi, buffer, size, HAL_MAX_DELAY);
    SX1272_Unselect(mod);
}

void SX1272_ReadBuffer(SX1272_t *mod, uint8_t addr, uint8_t *buffer, uint8_t size) {
    addr &= 0x7F;
    SX1272_Select(mod);
    HAL_SPI_Transmit(mod->hspi, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(mod->hspi, buffer, size, HAL_MAX_DELAY);
    SX1272_Unselect(mod);
}

void SX1272_Init(SX1272_t *mod, SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *nssP, uint16_t nssPin,
                 GPIO_TypeDef *rstP, uint16_t rstPin,
                 GPIO_TypeDef *dioP, uint16_t dioPin,
                 SX1272_Modulation_t modulation) {
    mod->hspi = hspi;
    mod->NSS_Port = nssP; mod->NSS_Pin = nssPin;
    mod->Reset_Port = rstP; mod->Reset_Pin = rstPin;
    mod->DIO0_Port = dioP; mod->DIO0_Pin = dioPin;
    mod->TX_SW_Port = NULL;
    mod->packetReceived = false;
    mod->modulation = modulation;

    // Initialization is called once at startup, no lock needed
    HAL_GPIO_WritePin(mod->Reset_Port, mod->Reset_Pin, GPIO_PIN_SET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(mod->Reset_Port, mod->Reset_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    SX1272_WriteReg(mod, REG_OP_MODE, SX1272_MODE_SLEEP);
    HAL_Delay(2);
    SX1272_WriteReg(mod, REG_OP_MODE, SX1272_MODE_SLEEP | (uint8_t)mod->modulation);
    HAL_Delay(2);
    SX1272_WriteReg(mod, REG_OP_MODE, SX1272_MODE_STDBY | (uint8_t)mod->modulation);
}

void SX1272_ConfigAntennaSwitch(SX1272_t *mod, GPIO_TypeDef *txP, uint16_t txPin, GPIO_TypeDef *rxP, uint16_t rxPin) {
    mod->TX_SW_Port = txP; mod->TX_SW_Pin = txPin;
    mod->RX_SW_Port = rxP; mod->RX_SW_Pin = rxPin;
}

void SX1272_Setup(SX1272_t *mod, uint32_t freq, uint8_t bw, uint8_t cr, uint8_t sf) {
    // LOCK SPI: Ensure setup completes without interruption
    __disable_irq();

    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    SX1272_WriteReg(mod, REG_FRF_MSB, (frf >> 16) & 0xFF);
    SX1272_WriteReg(mod, REG_FRF_MID, (frf >> 8)  & 0xFF);
    SX1272_WriteReg(mod, REG_FRF_LSB, frf & 0xFF);

    if (mod->modulation == SX1272_MOD_LORA) {
        SX1272_WriteReg(mod, REG_FIFO_TX_BASE_ADDR, 0x80);
        SX1272_WriteReg(mod, REG_FIFO_RX_BASE_ADDR, 0x00);
        SX1272_WriteReg(mod, REG_MODEM_CONFIG1, bw | cr | 0x02);
        SX1272_WriteReg(mod, REG_MODEM_CONFIG2, sf | 0x04);
    }
    SX1272_WriteReg(mod, REG_PA_CONFIG, 0x8F);
    SX1272_SetAntenna(mod, SX1272_MODE_STDBY);

    __enable_irq();
}

void SX1272_Transmit(SX1272_t *mod, uint8_t *data, uint8_t size) {
    // --- ATOMIC LOCK ---
    // Critical: Prevents RX Interrupt from firing during TX setup
    // This stops the "RX stops working" bug.
    __disable_irq();

    if (mod->modulation == SX1272_MOD_LORA) {
        SX1272_WriteReg(mod, REG_DIO_MAPPING1, 0x40); // DIO0 = TxDone
        SX1272_WriteReg(mod, REG_FIFO_ADDR_PTR, 0x80);
        SX1272_WriteReg(mod, REG_PAYLOAD_LENGTH, size);
        SX1272_WriteBuffer(mod, REG_FIFO, data, size);

        SX1272_SetAntenna(mod, SX1272_MODE_TX);
        SX1272_WriteReg(mod, REG_OP_MODE, SX1272_MODE_TX | (uint8_t)mod->modulation);
    }

    __enable_irq();
}

void SX1272_Receive(SX1272_t *mod) {
    // --- ATOMIC LOCK ---
    __disable_irq();

    if (mod->modulation == SX1272_MOD_LORA) {
        SX1272_WriteReg(mod, REG_DIO_MAPPING1, 0x00); // DIO0 = RxDone
        SX1272_WriteReg(mod, REG_FIFO_ADDR_PTR, 0x00);
    }

    // Clear flags before restarting
    SX1272_WriteReg(mod, REG_IRQ_FLAGS, 0xFF);

    SX1272_SetAntenna(mod, SX1272_MODE_RX_CONT);
    SX1272_WriteReg(mod, REG_OP_MODE, SX1272_MODE_RX_CONT | (uint8_t)mod->modulation);

    __enable_irq();
}

void SX1272_HandleDIO0(SX1272_t *mod) {
    // ISR context: already highest priority in this system.

    if (mod->modulation == SX1272_MOD_LORA) {
        uint8_t irq = SX1272_ReadReg(mod, REG_IRQ_FLAGS);
        SX1272_WriteReg(mod, REG_IRQ_FLAGS, 0xFF); // Clear

        if (irq & 0x40) { // RxDone

            // --- DROPPING LOGIC ---
            // If main.c hasn't cleared the flag yet, it means UART is still busy printing.
            // We MUST drop this new packet to prevent overwriting the buffer.
            if(mod->packetReceived) {
                return;
            }

            if (!(irq & 0x20)) { // No CRC Error

                // Freeze Radio to Standby
                SX1272_SetAntenna(mod, SX1272_MODE_STDBY);
                SX1272_WriteReg(mod, REG_OP_MODE, SX1272_MODE_STDBY | SX1272_MOD_LORA);

                mod->rxLength = SX1272_ReadReg(mod, REG_RX_NB_BYTES);
                uint8_t currentAddr = SX1272_ReadReg(mod, REG_FIFO_RX_CURRENT);
                SX1272_WriteReg(mod, REG_FIFO_ADDR_PTR, currentAddr);

                if (mod->rxLength > 0) {
                    // --- CLEAN BUFFER FIX ---
                    // Wipe buffer to ensure main.c doesn't print old garbage
                    memset(mod->rxBuffer, 0, 256);

                    SX1272_ReadBuffer(mod, REG_FIFO, mod->rxBuffer, mod->rxLength);
                    mod->rxBuffer[mod->rxLength] = '\0';
                    mod->packetReceived = true;
                }
            }
        } else if (irq & 0x08) { // TxDone
            SX1272_SetAntenna(mod, SX1272_MODE_STDBY);
            SX1272_WriteReg(mod, REG_OP_MODE, SX1272_MODE_STDBY | SX1272_MOD_LORA);
        }
    }
}
