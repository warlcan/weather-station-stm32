#ifndef BSP_H
#define BSP_H

#include "main.h"
#include <stdbool.h>

extern volatile uint32_t system_ticks;

#define WAIT_FLAG(wait_flag_function, timeout) ({\
    uint32_t _start = system_ticks;              \
    bool _result = true;                         \
    while (!(wait_flag_function)) {              \
        if(system_ticks - _start >= timeout) {   \
            _result = false;                     \
            break;                               \
        }                                        \
    }                                            \
    _result;                                     \
})

typedef enum {
    ERR_NO_ERROR      = 0x00U,
    ERR_SENSORS_ERROR = 1 << 0,
    ERR_I2C_BUSY      = 1 << 1,
    ERR_I2C_TXRX      = 1 << 2,
    ERR_I2C_STOP      = 1 << 3,
    ERR_NRF_NOT_FOUND = 1 << 4,
    ERR_NRF_BSY       = 1 << 5,
    ERR_NRF_TXRX      = 1 << 6,
} BSP_ErrMask_t;

void BSP_LowPowerDelay(uint32_t Delay);
void BSP_DelayUS(uint16_t delay_us);

uint32_t BSP_GetUID();
uint32_t BSP_GetRandNum(uint32_t *seed);

void BSP_ErrorSet(BSP_ErrMask_t error_mask);
void BSP_ErrorReset(BSP_ErrMask_t error_mask);
uint8_t BSP_GetErrors(void);

void BSP_PeriphModeActive();
void BSP_PeriphModeSleep();

#endif