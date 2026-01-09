#include "sx1272.h"
#include <string.h>

uint8_t SX1272_RxBuffer[256];
volatile uint8_t SX1272_RxLength = 0;

// Interna spremenljivka za shranjevanje trenutne modulacije (0x00=FSK ali 0x80=LoRa)
static uint8_t _currentModulation = SX1272_MOD_LORA;

static void SX1272_Select(void)   { HAL_GPIO_WritePin(SX1272_NSS_PORT, SX1272_NSS_PIN, GPIO_PIN_RESET); }
static void SX1272_Unselect(void) { HAL_GPIO_WritePin(SX1272_NSS_PORT, SX1272_NSS_PIN, GPIO_PIN_SET); }

void SX1272_Reset(void)
{
    HAL_GPIO_WritePin(SX1272_RESET_PORT, SX1272_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SX1272_RESET_PORT, SX1272_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(5);
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
    SX1272_WriteReg(REG_FRF_MSB, (frf >> 16) & 0xFF);
    SX1272_WriteReg(REG_FRF_MID, (frf >> 8)  & 0xFF);
    SX1272_WriteReg(REG_FRF_LSB, frf & 0xFF);
}

// --- Nova Setup Funkcija ---
void SX1272_Setup(uint32_t freq, uint8_t modulation, uint8_t bw, uint8_t cr, uint8_t sf)
{
    // Shranimo izbrano modulacijo za kasnejšo uporabo v Transmit/Receive
    _currentModulation = modulation;

    // 1. Postavi v SLEEP (nujno za preklop modulacije!)
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_SLEEP);
    HAL_Delay(10);

    // 2. Nastavi modulacijo (Bit 7: 0=FSK, 1=LoRa) v Sleep načinu
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_SLEEP | _currentModulation);
    HAL_Delay(10);

    // 3. Pojdi v STANDBY (ohrani bit modulacije)
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_STDBY | _currentModulation);

    // 4. Nastavi Frekvenco
    SX1272_SetFrequency(freq);

    // 5. Konfiguracija glede na modulacijo
    if (_currentModulation == SX1272_MOD_LORA)
    {
        // --- LoRa Konfiguracija ---
        SX1272_WriteReg(REG_FIFO_TX_BASE_ADDR, 0x00);
        SX1272_WriteReg(REG_FIFO_RX_BASE_ADDR, 0x00);

        // BW | CR | Header(Explicit) | CRC Enable(1)
        SX1272_WriteReg(REG_MODEM_CONFIG1, bw | cr | 0x02);

        // SF | AGC Auto On(1)
        SX1272_WriteReg(REG_MODEM_CONFIG2, sf | 0x04);
    }
    else
    {
        // --- FSK Konfiguracija (Osnovna) ---
        // Opomba: FSK zahteva nastavitev Bitrate, Fdev, Preamble itd.
        // Tu pustimo privzete vrednosti ali dodamo specifične registre za FSK,
        // če jih potrebujete. Za zdaj samo preklopimo način.
    }

    // Map DIO0 default (RxDone za LoRa, za FSK je to odvisno od PacketMode)
    SX1272_WriteReg(REG_DIO_MAPPING1, 0x00);
}

void SX1272_Init(uint32_t freq, uint8_t modulation, uint8_t bw, uint8_t cr, uint8_t sf)
{
    SX1272_Reset();
    SX1272_Setup(freq, modulation, bw, cr, sf);
}

void SX1272_Transmit(uint8_t *data, uint8_t size)
{
    // Mapiranje DIO0 na TxDone (01)
    SX1272_WriteReg(REG_DIO_MAPPING1, 0x40);

    // Standby
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_STDBY | _currentModulation);

    if (_currentModulation == SX1272_MOD_LORA) {
        SX1272_WriteReg(REG_FIFO_ADDR_PTR, 0x00);
        SX1272_WriteBuffer(REG_FIFO, data, size);
        SX1272_WriteReg(REG_PAYLOAD_LENGTH, size);
    } else {
        // FSK FIFO handling is slightly different (requires SyncWord setup etc.)
        // Za preprostost tu uporabljamo isto FIFO logiko, ki deluje za Packet Mode
        SX1272_WriteBuffer(REG_FIFO, data, size);
        // FSK nima REG_PAYLOAD_LENGTH na istem naslovu na isti način v vseh modih,
        // a za osnovni packet mode je podobno.
    }

    // Clear IRQ
    SX1272_WriteReg(REG_IRQ_FLAGS, 0xFF);

    // Start TX (uporabi _currentModulation, da ne povoziš LoRa bita!)
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_TX | _currentModulation);
}

void SX1272_Receive(void)
{
    SX1272_WriteReg(REG_DIO_MAPPING1, 0x00); // DIO0 -> RxDone
    SX1272_WriteReg(REG_IRQ_FLAGS, 0xFF);

    // Start RX Continuous
    SX1272_WriteReg(REG_OP_MODE, SX1272_MODE_RX_CONT | _currentModulation);
}

void SX1272_HandleDIO0(void)
{
    // Branje statusa
    uint8_t irqFlags = SX1272_ReadReg(REG_IRQ_FLAGS);
    SX1272_WriteReg(REG_IRQ_FLAGS, 0xFF); // Clear

    // Preverjanje zastavic (maske so za LoRa, FSK ima druge pomene bitov!)
    // Če uporabljate FSK, bi morali preveriti PayloadReady bit namesto RxDone.
    // Spodnja koda je optimizirana za LoRa.

    if (_currentModulation == SX1272_MOD_LORA)
    {
        if ((irqFlags & IRQ_RX_DONE_MASK) && !(irqFlags & IRQ_CRC_ERROR_MASK))
        {
            SX1272_RxLength = SX1272_ReadReg(REG_RX_NB_BYTES);
            SX1272_WriteReg(REG_FIFO_ADDR_PTR, SX1272_ReadReg(REG_FIFO_RX_CURRENT));
            if(SX1272_RxLength > 0) SX1272_ReadBuffer(REG_FIFO, SX1272_RxBuffer, SX1272_RxLength);
            if(SX1272_RxLength < 255) SX1272_RxBuffer[SX1272_RxLength] = '\0';
        }
        else if (irqFlags & IRQ_TX_DONE_MASK)
        {
            SX1272_Receive();
        }
    }
    else
    {
        // FSK Handling (poenostavljeno)
        // Pri FSK DIO0 običajno pomeni PayloadReady (PacketSent / PacketReceived)
        // Za popolno FSK podporo je potrebna dodatna logika tukaj.
    }
}
