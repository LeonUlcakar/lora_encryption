#include "sx1272.h"
#include <string.h>

uint8_t SX1272_RxBuffer[256];
volatile uint8_t SX1272_RxLength = 0;

static void SX1272_Select(void)   { HAL_GPIO_WritePin(SX1272_NSS_PORT, SX1272_NSS_PIN, GPIO_PIN_RESET); }
static void SX1272_Unselect(void) { HAL_GPIO_WritePin(SX1272_NSS_PORT, SX1272_NSS_PIN, GPIO_PIN_SET); }

void SX1272_Reset(void)
{
    HAL_GPIO_WritePin(SX1272_RESET_PORT, SX1272_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(1); // >100 µs
    HAL_GPIO_WritePin(SX1272_RESET_PORT, SX1272_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(5); // >5 ms
}

void SX1272_WriteReg(uint8_t addr, uint8_t data)
{
    addr |= 0x80; // MSB=1 for write
    SX1272_Select();
    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    SX1272_Unselect();
}

uint8_t SX1272_ReadReg(uint8_t addr)
{
    uint8_t value;
    SX1272_Select();
    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &value, 1, HAL_MAX_DELAY);
    SX1272_Unselect();
    return value;
}

void SX1272_WriteBuffer(uint8_t addr, uint8_t *buffer, uint8_t size)
{
    addr |= 0x80;
    SX1272_Select();
    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, buffer, size, HAL_MAX_DELAY);
    SX1272_Unselect();
}

void SX1272_ReadBuffer(uint8_t addr, uint8_t *buffer, uint8_t size)
{
    SX1272_Select();
    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, buffer, size, HAL_MAX_DELAY);
    SX1272_Unselect();
}

void SX1272_SetFrequency(uint32_t freq) {

    uint64_t frf = ((uint64_t)freq << 19) / 32000000;

    // Write to frequency registers
    SX1272_WriteReg(REG_FRF_MSB, (frf >> 16) & 0xFF);
    SX1272_WriteReg(REG_FRF_MID, (frf >> 8)  & 0xFF);
    SX1272_WriteReg(REG_FRF_LSB, frf & 0xFF);
}

void SX1272_SetupLora(void)
{
    // Sleep then LoRa mode
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_SLEEP | SX1272_MODE_LORA);
    HAL_Delay(10);

    // Standby
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_STDBY | SX1272_MODE_LORA);

    SX1272_SetFrequency(868000000);

    // Base addresses
    SX1272_WriteReg(REG_FIFO_TX_BASE_ADDR, 0x00);
    SX1272_WriteReg(REG_FIFO_RX_BASE_ADDR, 0x00);

    // Modem config (BW=125kHz, CR=4/5, SF=7)
    SX1272_WriteReg(REG_MODEM_CONFIG1, 0x72);
    SX1272_WriteReg(REG_MODEM_CONFIG2, 0x74);


    // Map DIO0: RxDone=00, TxDone=01 depending on mode
    SX1272_WriteReg(REG_DIO_MAPPING1, 0x00);
}

void SX1272_Init(void)
{
    SX1272_Reset();
    SX1272_SetupLora();
}

void SX1272_Transmit(uint8_t *data, uint8_t size)
{
    // Map DIO0 to TxDone
    SX1272_WriteReg(REG_DIO_MAPPING1, 0x40);

    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_STDBY | SX1272_MODE_LORA);
    SX1272_WriteReg(REG_FIFO_ADDR_PTR, 0x00);
    SX1272_WriteBuffer(REG_FIFO, data, size);
    SX1272_WriteReg(REG_PAYLOAD_LENGTH, size);

    SX1272_WriteReg(REG_IRQ_FLAGS, 0xFF); // Clear all IRQs
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_TX | SX1272_MODE_LORA);
}

void SX1272_Receive(void)
{
    // Map DIO0 to RxDone
    SX1272_WriteReg(REG_DIO_MAPPING1, 0x00);

    SX1272_WriteReg(REG_IRQ_FLAGS, 0xFF); // Clear all IRQs
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_RX_CONT | SX1272_MODE_LORA);
}

void SX1272_HandleDIO0(void)
{
    uint8_t irqFlags = SX1272_ReadReg(REG_IRQ_FLAGS);

    // Always clear ALL interrupts first
    SX1272_WriteReg(REG_IRQ_FLAGS, 0xFF);  // Clear all flags

    if (irqFlags & IRQ_RX_DONE_MASK)
    {
        if (!(irqFlags & IRQ_CRC_ERROR_MASK))
        {
            // 1. Get payload length FIRST
            SX1272_RxLength = SX1272_ReadReg(REG_RX_NB_BYTES);

            // 2. Get current FIFO address
            uint8_t currentAddr = SX1272_ReadReg(REG_FIFO_RX_CURRENT);

            // 3. Set FIFO pointer
            SX1272_WriteReg(REG_FIFO_ADDR_PTR, currentAddr);

            // 4. Read FIFO
            if(SX1272_RxLength > 0 && SX1272_RxLength <= 256) {
                SX1272_ReadBuffer(REG_FIFO, SX1272_RxBuffer, SX1272_RxLength);
            }

            // Optional: Null terminate if needed
            if (SX1272_RxLength < 255) {
                SX1272_RxBuffer[SX1272_RxLength] = '\0';
            }
        }
    }
    else if (irqFlags & IRQ_TX_DONE_MASK)
    {
        // Immediately return to RX mode after TX
        SX1272_Receive();
    }
}

