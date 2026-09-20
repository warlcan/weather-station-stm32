#include "bsp_periph.h"
#include "bsp.h"

// === SENSORS ===

static void BSP_SensorStart(void) {
    LL_GPIO_SetOutputPin(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin);
    LL_GPIO_SetPinMode(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin, LL_GPIO_MODE_OUTPUT);

    BSP_LowPowerDelay(100);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_ALTERNATE);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    LL_I2C_Enable(I2C1);
}

static void BSP_SensorStop(void) {
    LL_I2C_Disable(I2C1);
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);
    
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_9, LL_GPIO_MODE_ANALOG);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_ANALOG);

    LL_GPIO_ResetOutputPin(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin);
    LL_GPIO_SetPinMode(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin, LL_GPIO_MODE_ANALOG);
}

// === SPI ===

static void BSP_SpiStart(void) {
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
    LL_SPI_Enable(SPI1); 
}

static void BSP_SpiStop(void){
    LL_SPI_Disable(SPI1);
    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_SPI1);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_6, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);

    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_5);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_6);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
    
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_5, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_6, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_7, LL_GPIO_PULL_NO);
}

// === PERIPHERAL MODES ===

void BSP_PeriphModeActive() {
    BSP_SensorStart();
    BSP_SpiStart();
}

void BSP_PeriphModeSleep(){
    BSP_SensorStop();
    BSP_SpiStop();
}