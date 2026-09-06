#include "bsp.h"

#include "stm32l0xx_ll_exti.h"

#define SPIx SPI1
#define I2Cx I2C1
#define LPTIMx LPTIM1

extern volatile uint32_t system_ticks;

// === ERROR HANDLERS ===

static volatile uint8_t system_errors = ERR_NO_ERROR;

void BSP_ErrorSet(BSP_ErrMask_t error_mask) {system_errors |= error_mask;}
void BSP_ErrorReset(BSP_ErrMask_t error_mask) {system_errors &= ~error_mask;}
uint8_t BSP_GetErrors(void) {return system_errors;}

// === SYSTEM ===

void BSP_LowPowerDelay(uint32_t delay_ms) {
    if (delay_ms == 0) return;

    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29);

    uint32_t ticks = (delay_ms * 1156) / 1000;
    if (ticks == 0) ticks = 1;
    if (ticks > 65535) ticks = 65535;

    LL_LPTIM_Enable(LPTIMx);
    LL_LPTIM_SetAutoReload(LPTIMx, ticks);
    
    WAIT_FLAG(LL_LPTIM_IsActiveFlag_ARROK(LPTIMx), 5); 
    LL_LPTIM_ClearFlag_ARROK(LPTIMx);

    LL_LPTIM_ClearFlag_ARRM(LPTIMx);
    LL_LPTIM_EnableIT_ARRM(LPTIMx);

    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    
    LL_LPTIM_StartCounter(LPTIMx, LL_LPTIM_OPERATING_MODE_ONESHOT);
    
    LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);
    LL_LPM_EnableDeepSleep(); 

    __disable_irq(); 
    if (!LL_LPTIM_IsActiveFlag_ARRM(LPTIMx)) {
        __WFI();
    }
    __enable_irq();

    LL_LPM_EnableSleep(); 

    LL_LPTIM_DisableIT_ARRM(LPTIMx);
    LL_LPTIM_Disable(LPTIMx);
    
    system_ticks += delay_ms;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
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
    LL_I2C_Enable(I2Cx);
}

static void BSP_SensorStop(void) {
    LL_I2C_Disable(I2Cx);
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
    LL_SPI_Enable(SPIx); 
}

static void BSP_SpiStop(void){
    LL_SPI_Disable(SPIx);
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