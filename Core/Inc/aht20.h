#ifndef AHT20_H
#define AHT20_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32l0xx_ll_i2c.h"
#include "i2c_utils.h"

typedef struct {
    float humidity;
    float temperature;
} AHT20_Data_t;

bool AHT20_GetData(I2C_TypeDef *I2Cx, AHT20_Data_t *out_data);

#endif