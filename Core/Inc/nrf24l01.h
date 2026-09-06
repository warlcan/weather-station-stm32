#ifndef NRF24L01_H
#define NRF24L01_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "bsp.h"
#include "stm32l0xx_ll_spi.h"
#include "stm32l0xx_ll_gpio.h"

#define NRF24_CSN_PORT    NRF_CSN_GPIO_Port
#define NRF24_CSN_PIN     NRF_CSN_Pin

#define NRF24_CE_PORT     NRF_CE_GPIO_Port
#define NRF24_CE_PIN      NRF_CE_Pin

typedef struct __attribute__((packed)) {
    uint32_t uid;
    float temperature;
    float humidity;
    float pressure;
    uint8_t errors;
} NRF24_Data_t;

void NRF24_Init(SPI_TypeDef *SPIx);
bool NRF24_TransmitData(SPI_TypeDef *SPIx, NRF24_Data_t *nrf24_data, uint8_t nrf24_data_size);

#endif