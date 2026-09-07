/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32l0xx_ll_pwr.h"
#include "stm32l0xx_ll_exti.h"
#include "stm32l0xx_ll_cortex.h"
#include "SEGGER_RTT.h"
#include "aht20.h"
#include "bmp280.h"
#include "nrf24l01.h"
#include "bsp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// #define debug
#define SLEEP_ONE_PHASE_TIME_MS 20000
#define SLEEP_PHASES_NUMBER     30
#define SLEEP_JITTER_LIMIT      10000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#ifdef debug
    #define DEBUG_RTT_Init()                        SEGGER_RTT_Init()        
    #define DEBUG_RTT_WriteString(buffer_index, s)  SEGGER_RTT_WriteString(buffer_index, s)
    #define DEBUG_RTT_PutChar(buffer_index, s)      SEGGER_RTT_PutChar(buffer_index, s)
#else
    #define DEBUG_RTT_Init()
    #define DEBUG_RTT_WriteInt(buffer_index, num)
    #define DEBUG_RTT_WriteString(buffer_index, s)
    #define DEBUG_RTT_PutChar(buffer_index, s)
#endif
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t system_ticks = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
void MX_SPI1_Init(void);
static void MX_IWDG_Init(void);
static void MX_LPTIM1_Init(void);
/* USER CODE BEGIN PFP */
#ifdef debug
void DEBUG_RTT_WriteInt(uint8_t buffer_index, int num);
#endif
void BSP_LowPowerDelay(uint32_t Delay);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  AHT20_Data_t aht20_data;
  BMP280_Data_t bmp280_data;
  NRF24_Data_t nrf24_data;
  uint32_t wakeup_counter = 0;
  uint32_t rand_num = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, 3);

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  LL_SYSTICK_EnableIT();
  LL_mDelay(2000); //SWD escape window to prevent bricking
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_IWDG_Init();
  MX_LPTIM1_Init();
  /* USER CODE BEGIN 2 */
  LL_PWR_EnableUltraLowPower();
  LL_PWR_EnableFastWakeUp();

  rand_num = BSP_GetUID();

  #ifdef debug
  DEBUG_RTT_Init();
  DEBUG_RTT_WriteString(0, "RTT initialized.\r\n");
  DEBUG_RTT_WriteString(0, "RCC-CSR: 0x");
  DEBUG_RTT_WriteInt(0, RCC->CSR);
  DEBUG_RTT_PutChar(0, '\n');
  LL_mDelay(1000);
  RCC->CSR |= RCC_CSR_RMVF;
  #endif

  LL_LPTIM_Enable(LPTIM1);
  LL_LPTIM_EnableIT_ARRM(LPTIM1);
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29);

  BSP_PeriphModeActive();
  NRF24_Init(SPI1);   DEBUG_RTT_WriteString(0, "NRF Init.\r\n");
  BMP280_GetCoef(I2C1);  DEBUG_RTT_WriteString(0, "BMP Init.\r\n");
  BSP_PeriphModeSleep();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    BSP_PeriphModeActive();

    if(!AHT20_GetData(I2C1, &aht20_data)) {
      BSP_ErrorSet(ERR_SENSORS_ERROR)
      DEBUG_RTT_WriteString(0, "AHT20 Error\n");
    }

    if(!BMP280_GetData(I2C1, &bmp280_data)) {
      BSP_ErrorSet(ERR_SENSORS_ERROR)
      DEBUG_RTT_WriteString(0, "BMP280 Error\n");
    }

    nrf24_data.uid         = BSP_GetUID();
    nrf24_data.temperature = aht20_data.temperature;
    nrf24_data.humidity    = aht20_data.humidity;
    nrf24_data.pressure    = bmp280_data.pressure;
    nrf24_data.errors      = BSP_GetErrors();

    NRF24_TransmitData(SPI1, &nrf24_data, sizeof(nrf24_data));
    BSP_PeriphModeSleep();
    #ifdef debug
    while (SEGGER_RTT_HasDataUp(0) != 0);
    #endif

    wakeup_counter = 0;
    while (wakeup_counter < SLEEP_PHASES_NUMBER) {
      LL_IWDG_ReloadCounter(IWDG);
      
      BSP_LowPowerDelay(SLEEP_ONE_PHASE_TIME_MS);

      wakeup_counter++;
    }
    LL_IWDG_ReloadCounter(IWDG);
    uint32_t jitter_sleep_time = (uint32_t)(((uint64_t)BSP_GetRandNum(&rand_num) * SLEEP_JITTER_LIMIT) >> 32);
    BSP_LowPowerDelay(jitter_sleep_time);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  while (LL_PWR_IsActiveFlag_VOS() != 0)
  {
  }
  LL_RCC_LSI_Enable();

   /* Wait till LSI is ready */
  while(LL_RCC_LSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_Enable();

   /* Wait till MSI is ready */
  while(LL_RCC_MSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_5);
  LL_RCC_MSI_SetCalibTrimming(0);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
  {

  }

  LL_Init1msTick(2097000);

  LL_SetSystemCoreClock(2097000);
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);
  LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSI);
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**I2C1 GPIO Configuration
  PA9   ------> I2C1_SCL
  PA10   ------> I2C1_SDA
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */

  /** I2C Initialization
  */
  LL_I2C_EnableAutoEndMode(I2C1);
  LL_I2C_DisableOwnAddress2(I2C1);
  LL_I2C_DisableGeneralCall(I2C1);
  LL_I2C_EnableClockStretching(I2C1);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x00000608;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  LL_IWDG_Enable(IWDG);
  LL_IWDG_EnableWriteAccess(IWDG);
  LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_256);
  LL_IWDG_SetReloadCounter(IWDG, 4095);
  while (LL_IWDG_IsReady(IWDG) != 1)
  {
  }

  LL_IWDG_ReloadCounter(IWDG);
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief LPTIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPTIM1_Init(void)
{

  /* USER CODE BEGIN LPTIM1_Init 0 */

  /* USER CODE END LPTIM1_Init 0 */

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);

  /* LPTIM1 interrupt Init */
  NVIC_SetPriority(LPTIM1_IRQn, 0);
  NVIC_EnableIRQ(LPTIM1_IRQn);

  /* USER CODE BEGIN LPTIM1_Init 1 */

  /* USER CODE END LPTIM1_Init 1 */
  LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
  LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV32);
  LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
  LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_ENDOFPERIOD);
  LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
  LL_LPTIM_TrigSw(LPTIM1);
  /* USER CODE BEGIN LPTIM1_Init 2 */

  /* USER CODE END LPTIM1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**SPI1 GPIO Configuration
  PA5   ------> SPI1_SCK
  PA6   ------> SPI1_MISO
  PA7   ------> SPI1_MOSI
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 7;
  LL_SPI_Init(SPI1, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
  /* USER CODE BEGIN SPI1_Init 2 */
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_6, LL_GPIO_PULL_DOWN);
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_SetOutputPin(NRF_CSN_GPIO_Port, NRF_CSN_Pin);

  /**/
  LL_GPIO_ResetOutputPin(NRF_CE_GPIO_Port, NRF_CE_Pin);

  /**/
  LL_GPIO_ResetOutputPin(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin);

  /**/
  GPIO_InitStruct.Pin = NRF_CSN_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(NRF_CSN_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = NRF_CE_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(NRF_CE_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = SENSOR_VDD_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(SENSOR_VDD_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ANALOG);
  
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_ANALOG);
  #ifndef debug
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_13, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);
  #endif
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
#ifdef debug
void DEBUG_RTT_WriteInt(uint8_t buffer_index, int num) {
  if (num == 0) { DEBUG_RTT_PutChar(buffer_index, '0'); return; }
  if (num < 0)  { DEBUG_RTT_PutChar(buffer_index, '-'); num = -num; }
  if (num >= 10){ DEBUG_RTT_WriteInt(buffer_index, num / 10); }
  DEBUG_RTT_PutChar(buffer_index, (num % 10) + '0');
}
#endif

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
