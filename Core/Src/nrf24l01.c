#include "nrf24l01.h"

#define NRF24_WAKEUP_DELAY_MS      2
#define NRF24_CE_DELAY_US          20
#define SPI_TIMEOUT_MS             5

typedef enum {
    NRF24_REG_CONFIG      = 0x00,
    NRF24_REG_EN_AA       = 0x01,
    NRF24_REG_EN_RXADDR   = 0x02,
    NRF24_REG_SETUP_AW    = 0x03,
    NRF24_REG_SETUP_RETR  = 0x04,
    NRF24_REG_RF_CH       = 0x05,
    NRF24_REG_RF_SETUP    = 0x06,
    NRF24_REG_STATUS      = 0x07,
    NRF24_REG_OBSERVE_TX  = 0x08,
    NRF24_REG_RPD         = 0x09,

    NRF24_REG_TX_ADDR     = 0x10
} Nrf24RegAddr_t;

// === 0x00 CONFIG  ===

#define NRF24_CONFIG_PRIM         0x00
#define NRF24_CONFIG_PWR_UP      (1 << 1)
#define NRF24_CONFIG_CRCO        (1 << 2)
#define NRF24_CONFIG_EN_CRC      (1 << 3)
#define NRF24_CONFIG_MASK_MAX_RT (1 << 4)
#define NRF24_CONFIG_MASK_TX_DS  (1 << 5)
#define NRF24_CONFIG_MASK_RX_DR  (1 << 6)

#define NRF24_CONFIG_POWER_UP  (NRF24_CONFIG_PRIM | NRF24_CONFIG_PWR_UP |\
                                NRF24_CONFIG_CRCO | NRF24_CONFIG_EN_CRC)                             
#define NRF24_CONFIG_POWER_DOWN 0x00

// === 0x03 ADDRESS WIDTH ===

#define NRF24_AW_3BYTES    0x01
#define NRF24_AW_4BYTES    0x02
#define NRF24_AW_5BYTES    0x03

// === 0x06 TX POWER ===

#define NRF24_PWR_MINUS_18DBM (0x00 << 1)
#define NRF24_PWR_MINUS_12DBM (0x01 << 1)
#define NRF24_PWR_MINUS_6DBM  (0x02 << 1)
#define NRF24_PWR_0DBM        (0x03 << 1)

// === 0x06 TX SPEED === 

#define NRF24_DR_1MBPS         0x00
#define NRF24_DR_2MBPS        (1 << 3)
#define NRF24_DR_250KBPS      (1 << 5)

// === 0x07 STATUS  ===

#define NRF24_STATUS_TX_FULL_MASK  (1 << 0) 
#define NRF24_STATUS_RX_P_NO_MASK  (0x07 << 1)
#define NRF24_STATUS_MAX_RT_MASK   (1 << 4) 
#define NRF24_STATUS_TX_DS_MASK    (1 << 5) 
#define NRF24_STATUS_RX_DR_MASK    (1 << 6) 
#define NRF24_STATUS_RX_EMPTY_MASK (1 << 7)

#define NRF24_STATUS_CLEAR_ALL  (NRF24_STATUS_MAX_RT_MASK |\
                                 NRF24_STATUS_TX_DS_MASK  |\
                                 NRF24_STATUS_RX_DR_MASK)

// === COMMANDS ===

#define NRF24_CMD_R_REGISTER    0x00
#define NRF24_CMD_W_REGISTER    0x20
#define NRF24_CMD_W_TX_PAYLOAD  0xA0
#define NRF24_CMD_R_RX_PAYLOAD  0x61
#define NRF24_CMD_FLUSH_TX      0xE1
#define NRF24_CMD_FLUSH_RX      0xE2

//US delay for 2.1 MHz
#define NRF24_DELAY_US(us) do {                 \
    volatile uint32_t cycles = ((us) * 7) / 10; \
    if (cycles == 0) cycles = 1;                \
    while(cycles--);                            \
} while(0)

static uint8_t NRF24_SPI_WriteByte(SPI_TypeDef *SPIx, uint8_t data) {
    if (LL_SPI_IsActiveFlag_OVR(SPIx)) {
        LL_SPI_ReceiveData8(SPIx);
    }
    
    WAIT_FLAG(LL_SPI_IsActiveFlag_TXE(SPIx), SPI_TIMEOUT_MS);
    LL_SPI_TransmitData8(SPIx, data);
    
    WAIT_FLAG(LL_SPI_IsActiveFlag_RXNE(SPIx), SPI_TIMEOUT_MS);
    return LL_SPI_ReceiveData8(SPIx);
}

static void NRF24_SetReg(SPI_TypeDef *SPIx, Nrf24RegAddr_t reg, uint8_t val) {
    LL_GPIO_ResetOutputPin(NRF_CSN_GPIO_Port, NRF_CSN_Pin);
    NRF24_SPI_WriteByte(SPIx, NRF24_CMD_W_REGISTER | (reg & 0x1F));
    NRF24_SPI_WriteByte(SPIx, val);
    WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), SPI_TIMEOUT_MS);
    LL_GPIO_SetOutputPin(NRF_CSN_GPIO_Port, NRF_CSN_Pin);
}

static uint8_t NRF24_ReadReg(SPI_TypeDef *SPIx, Nrf24RegAddr_t reg) {
    uint8_t val;
    LL_GPIO_ResetOutputPin(NRF_CSN_GPIO_Port, NRF_CSN_Pin);
    NRF24_SPI_WriteByte(SPIx, reg & 0x1F);
    val = NRF24_SPI_WriteByte(SPIx, 0xFF); //0xFF - dummy byte
    WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), SPI_TIMEOUT_MS);
    LL_GPIO_SetOutputPin(NRF_CSN_GPIO_Port, NRF_CSN_Pin);
    return val;
}

static void NRF24_WriteByteBuf(SPI_TypeDef *SPIx, uint8_t cmd, uint8_t *buf, uint8_t buf_size) {
    LL_GPIO_ResetOutputPin(NRF_CSN_GPIO_Port, NRF_CSN_Pin);
    NRF24_SPI_WriteByte(SPIx, cmd);
    for(uint8_t i = 0; i < buf_size; i++) {
        NRF24_SPI_WriteByte(SPIx, buf[i]);
    }
    WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), SPI_TIMEOUT_MS);
    LL_GPIO_SetOutputPin(NRF_CSN_GPIO_Port, NRF_CSN_Pin);
}

void NRF24_Init(SPI_TypeDef *SPIx){
    NRF24_SetReg(SPIx, NRF24_REG_EN_AA, 0x00);
    NRF24_SetReg(SPIx, NRF24_REG_SETUP_RETR, 0x00);
    NRF24_SetReg(SPIx, NRF24_REG_SETUP_AW, NRF24_AW_5BYTES);
    NRF24_SetReg(SPIx, NRF24_REG_RF_CH, 100);

    NRF24_SetReg(SPIx, NRF24_REG_RF_SETUP, NRF24_DR_1MBPS | NRF24_PWR_0DBM);

    uint8_t addr[5] = {0x9c, 0x96, 0xf1, 0x1f, 0x5e}; //receiver address
    NRF24_WriteByteBuf(SPIx, NRF24_CMD_W_REGISTER | NRF24_REG_TX_ADDR, addr, sizeof(addr));
    
    NRF24_SetReg(SPIx, NRF24_REG_STATUS, NRF24_STATUS_CLEAR_ALL);

    uint8_t check_aw = NRF24_ReadReg(SPIx, NRF24_REG_SETUP_AW);
    if (check_aw != NRF24_AW_5BYTES) BSP_ErrorSet(ERR_NRF_NOT_FOUND);

}

bool NRF24_TransmitData(SPI_TypeDef *SPIx, NRF24_Data_t *nrf24_data, uint8_t nrf24_data_size) {
    NRF24_SetReg(SPIx, NRF24_REG_CONFIG, NRF24_CONFIG_POWER_UP);
    BSP_LowPowerDelay(NRF24_WAKEUP_DELAY_MS);

    NRF24_WriteByteBuf(SPIx, NRF24_CMD_W_TX_PAYLOAD, (uint8_t*)nrf24_data, nrf24_data_size);
    LL_GPIO_SetOutputPin(NRF_CE_GPIO_Port, NRF_CE_Pin);
    NRF24_DELAY_US(NRF24_CE_DELAY_US);
    LL_GPIO_ResetOutputPin(NRF_CE_GPIO_Port, NRF_CE_Pin);

    if(!WAIT_FLAG(NRF24_ReadReg(SPIx, NRF24_REG_STATUS) & NRF24_STATUS_TX_DS_MASK, SPI_TIMEOUT_MS)) {
            NRF24_SetReg(SPIx, NRF24_REG_STATUS, NRF24_STATUS_CLEAR_ALL);
            return false;
    }

    NRF24_SetReg(SPIx, NRF24_REG_STATUS, NRF24_STATUS_TX_DS_MASK);
    NRF24_SetReg(SPIx, NRF24_REG_CONFIG, NRF24_CONFIG_POWER_DOWN);
    return true;
}