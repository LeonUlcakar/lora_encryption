/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sx1272.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
SX1272_t lora_tx; // Dedicated Transmitter Module
SX1272_t lora_rx; // Dedicated Receiver Module
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- CONFIGURATION START ---

// UNCOMMENT THIS LINE FOR BOARD "A"


#define MASTER_BOARD

#ifdef MASTER_BOARD
    #define TX_FREQ 868100000 // 868.1 MHz
    #define RX_FREQ 868500000 // 868.5 MHz
    const char* my_msg = "Ping from Master";
#else
    #define TX_FREQ 868500000 // 868.5 MHz (Swapped)
    #define RX_FREQ 868100000 // 868.1 MHz (Swapped)
    const char* my_msg = "Pong from Slave";
#endif

// --- CONFIGURATION END ---
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
extern uint8_t SX1272_RxBuffer[256];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
uint8_t txBuffer[256];
uint8_t rxBuffer[256];
uint8_t status;
uint8_t irqFlags;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Check which module triggered the interrupt
    if (GPIO_Pin == lora_tx.DIO0_Pin) {
        SX1272_HandleDIO0(&lora_tx);
    }
    else if (GPIO_Pin == lora_rx.DIO0_Pin) {
        SX1272_HandleDIO0(&lora_rx);
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  // --- Initialize LORA 1 (Our Transmitter) ---
    SX1272_Init(&lora_tx, &hspi1,
                LORA1_NSS_GPIO_Port, LORA1_NSS_Pin,
                LORA1_RST_GPIO_Port, LORA1_RST_Pin,
                LORA1_DIO0_GPIO_Port, LORA1_DIO0_Pin);

    SX1272_ConfigAntennaSwitch(&lora_tx,
                LORA1_TX_SW_GPIO_Port, LORA1_TX_SW_Pin,
                LORA1_RX_SW_GPIO_Port, LORA1_RX_SW_Pin);

    // Setup TX Frequency
    SX1272_Setup(&lora_tx, TX_FREQ, SX1272_BW_125, SX1272_CR_4_5, SX1272_SF_7);

    // --- Initialize LORA 2 (Our Receiver) ---
    SX1272_Init(&lora_rx, &hspi1,
                LORA2_NSS_GPIO_Port, LORA2_NSS_Pin,
                LORA2_RST_GPIO_Port, LORA2_RST_Pin,
                LORA2_DIO0_GPIO_Port, LORA2_DIO0_Pin);

    SX1272_ConfigAntennaSwitch(&lora_rx,
                LORA2_TX_SW_GPIO_Port, LORA2_TX_SW_Pin,
                LORA2_RX_SW_GPIO_Port, LORA2_RX_SW_Pin);

    // Setup RX Frequency
    SX1272_Setup(&lora_rx, RX_FREQ, SX1272_BW_125, SX1272_CR_4_5, SX1272_SF_7);

    // Start Listening on the RX Module
    SX1272_Receive(&lora_rx);

    uint32_t last_send_time = 0;
    uint32_t counter = 0;
    uint8_t ver1 = SX1272_ReadReg(&lora_tx, 0x42); // Read Version Reg
    uint8_t ver2 = SX1272_ReadReg(&lora_rx, 0x42);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // --- TRANSMIT LOGIC (Runs on lora_tx) ---
	        if (HAL_GetTick() - last_send_time >= 1000) // Every 1 second
	        {
	            sprintf((char*)txBuffer, "%s #%lu", my_msg, counter++);
	            SX1272_Transmit(&lora_tx, txBuffer, strlen((char*)txBuffer));
	            last_send_time = HAL_GetTick();
	        }
	        ver1 = SX1272_ReadReg(&lora_tx, 0x42); // Read Version Reg
	        ver2 = SX1272_ReadReg(&lora_rx, 0x42);

	        // --- RECEIVE LOGIC (Runs on lora_rx) ---
	        if (lora_rx.packetReceived)
	        {
	            lora_rx.packetReceived = false;
	            // Process lora_rx.rxBuffer here if needed

	            // Go back to listening
	            SX1272_Receive(&lora_rx);
	        }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 21;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LORA1_RST_Pin|LORA2_RST_Pin|LORA1_DIO1_Pin|LORA1_RX_SW_Pin
                          |LORA2_TX_SW_Pin|LORA2_RX_SW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LORA2_NSS_GPIO_Port, LORA2_NSS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LORA1_TX_SW_GPIO_Port, LORA1_TX_SW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LORA1_NSS_GPIO_Port, LORA1_NSS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LORA2_DIO1_GPIO_Port, LORA2_DIO1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LORA1_RST_Pin LORA2_RST_Pin LORA1_DIO1_Pin LORA1_RX_SW_Pin
                           LORA2_TX_SW_Pin LORA2_RX_SW_Pin */
  GPIO_InitStruct.Pin = LORA1_RST_Pin|LORA2_RST_Pin|LORA1_DIO1_Pin|LORA1_RX_SW_Pin
                          |LORA2_TX_SW_Pin|LORA2_RX_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LORA1_DIO0_Pin LORA2_DIO0_Pin */
  GPIO_InitStruct.Pin = LORA1_DIO0_Pin|LORA2_DIO0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LORA2_NSS_Pin */
  GPIO_InitStruct.Pin = LORA2_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LORA2_NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LORA1_TX_SW_Pin */
  GPIO_InitStruct.Pin = LORA1_TX_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LORA1_TX_SW_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LORA1_NSS_Pin */
  GPIO_InitStruct.Pin = LORA1_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LORA1_NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LORA2_DIO1_Pin */
  GPIO_InitStruct.Pin = LORA2_DIO1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LORA2_DIO1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
