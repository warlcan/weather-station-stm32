#ifndef BMP280_H
#define BMP280_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32l0xx_ll_i2c.h"
#include "i2c_utils.h"

typedef struct {
    float pressure;
    float temperature;
} BMP280_Data_t;

bool BMP280_GetCoef(I2C_TypeDef *I2Cx);
bool BMP280_GetData(I2C_TypeDef *I2Cx, BMP280_Data_t *out_data);

#endif