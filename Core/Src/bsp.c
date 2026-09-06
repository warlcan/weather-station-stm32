#include "bsp.h"

#include "stm32l0xx_ll_exti.h"

extern volatile uint32_t system_ticks;

// === ERROR HANDLERS ===

static volatile uint8_t system_errors = ERR_NO_ERROR;

void BSP_ErrorSet(BSP_ErrMask_t error_mask) {system_errors |= error_mask;}
void BSP_ErrorReset(BSP_ErrMask_t error_mask) {system_errors &= ~error_mask;}
uint8_t BSP_GetErrors(void) {return system_errors;}

// === SYSTEM ===

void BSP_LowPowerDelay(uint32_t delay_ms) {
    if (delay_ms == 0) return;

    uint32_t ticks = (delay_ms * 1156) / 1000;
    if (ticks == 0) ticks = 1;
    if (ticks > 65535) ticks = 65535;

    LL_LPTIM_SetAutoReload(LPTIM1, ticks);
    WAIT_FLAG(LL_LPTIM_IsActiveFlag_ARROK(LPTIM1), 5); 
    LL_LPTIM_ClearFlag_ARROK(LPTIM1);
    LL_LPTIM_ClearFlag_ARRM(LPTIM1);

    LL_SYSTICK_DisableIT();
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_ONESHOT);

    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);
    LL_LPM_EnableDeepSleep();

    __disable_irq(); 
    if (!LL_LPTIM_IsActiveFlag_ARRM(LPTIM1)) {
        __WFI();
    }
    __enable_irq();

    LL_LPM_EnableSleep();  
    system_ticks += delay_ms;
    LL_SYSTICK_EnableIT();
}


uint32_t BSP_GetUID(void) {
    uint32_t *uid = (uint32_t *)UID_BASE;
    return uid[0] ^ uid[1] ^ uid[2];
}

// === SENSORS ===

static void BSP_SensorStart(void) {
    LL_GPIO_SetPinMode(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin);

    BSP_LowPowerDelay(50);

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

    LL_GPIO_SetPinMode(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin, LL_GPIO_MODE_ANALOG);
    LL_GPIO_ResetOutputPin(SENSOR_VDD_GPIO_Port, SENSOR_VDD_Pin);
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