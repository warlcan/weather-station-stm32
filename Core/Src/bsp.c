#include "bsp.h"

#include "stm32l0xx_ll_exti.h"

#define BSP_ADC_FLAG_TIMEOUT 5

#define LSI_FREQ_HZ          37000U
#define RTC_WUT_DIV          16U

#define RTC_WUT_TICKS_PER_SEC  (LSI_FREQ_HZ / RTC_WUT_DIV)
// === ERROR HANDLERS ===

static volatile uint8_t system_errors = ERR_NO_ERROR;

void BSP_ErrorSet(BSP_ErrMask_t error_mask) {system_errors |= error_mask;}
void BSP_ErrorReset(BSP_ErrMask_t error_mask) {system_errors &= ~error_mask;}
uint8_t BSP_GetErrors(void) {return system_errors;}

// === SYSTEM ===

void BSP_LowPowerDelay(uint32_t delay_ms) {
    if (delay_ms == 0) return;
    LL_RTC_ClearFlag_WUT(RTC);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_20);

    uint32_t ticks = ((uint64_t)delay_ms * RTC_WUT_TICKS_PER_SEC + 999U) / 1000U;
    if (ticks == 0) ticks = 1;
    if (ticks > 0xFFFFU) ticks = 0xFFFFU;

    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_WAKEUP_SetAutoReload(RTC, ticks - 1);
    LL_RTC_WAKEUP_Enable(RTC);
    LL_RTC_EnableWriteProtection(RTC);

    LL_SYSTICK_DisableIT();

    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);
    LL_LPM_EnableDeepSleep();

    __disable_irq();
        __WFI();
    __enable_irq(); 

    LL_LPM_EnableSleep();

    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_WAKEUP_Disable(RTC);
    LL_RTC_EnableWriteProtection(RTC);

    system_ticks += delay_ms;
    LL_SYSTICK_EnableIT();

}

void BSP_DelayUS(uint16_t delay_us) {
    if(delay_us == 0) return;

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM22);
    LL_TIM_SetCounter(TIM22, 0);
    LL_TIM_EnableCounter(TIM22);

    while(LL_TIM_GetCounter(TIM22) < delay_us);
    
    LL_TIM_DisableCounter(TIM22);
    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_TIM22);    
}

uint32_t BSP_GetUID(void) {
    uint32_t *uid = (uint32_t *)UID_BASE;
    return uid[0] ^ uid[1] ^ uid[2];
}

uint32_t BSP_GetRandNum(uint32_t *seed){
    if (*seed == 0) *seed = 0xA2E5; 
    uint32_t x = *seed; 

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    *seed = x;
    return x;
}

uint32_t BSP_GetVoltageLevel(){
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);
    LL_ADC_EnableInternalRegulator(ADC1);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_VREFINT);
    BSP_DelayUS(50);

    if (!LL_ADC_IsCalibrationOnGoing(ADC1)) {
        LL_ADC_StartCalibration(ADC1);
        WAIT_FLAG(!LL_ADC_IsCalibrationOnGoing(ADC1), BSP_ADC_FLAG_TIMEOUT);
    }

    LL_ADC_Enable(ADC1);
    WAIT_FLAG(LL_ADC_IsActiveFlag_ADRDY(ADC1), BSP_ADC_FLAG_TIMEOUT);

    LL_ADC_REG_StartConversion(ADC1);
    WAIT_FLAG(LL_ADC_IsActiveFlag_EOC(ADC1), BSP_ADC_FLAG_TIMEOUT);

    uint16_t vrefint_raw = LL_ADC_REG_ReadConversionData12(ADC1);
    uint32_t vdd_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(vrefint_raw, LL_ADC_RESOLUTION_12B);
    LL_ADC_ClearFlag_EOC(ADC1);

    LL_ADC_Disable(ADC1);
    WAIT_FLAG(!LL_ADC_IsEnabled(ADC1), BSP_ADC_FLAG_TIMEOUT);
    LL_ADC_DisableInternalRegulator(ADC1);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_NONE);
    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_ADC1);
    return vdd_mv;
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