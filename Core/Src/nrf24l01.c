#include "nrf24l01.h"

#define NRF24_WAKEUP_DELAY_MS   2
#define NRF24_CE_DELAY_US       20
#define SPI_TIMEOUT_MS          5

extern uint32_t SystemCoreClock;

#define DUMMY_BYTE                 0xFF

typedef enum {
    NRF24_REG_CONFIG      = 0x00,
    NRF24_REG_EN_AA       = 0x01,
    NRF24_REG_EN_RXADDR   = 0x02,
    NRF24_REG_SETUP_AW    = 0x03,
    NRF24_REG_SETUP_RETR  = 0x04,
    NRF24_REG_RF_CH       = 0x05,
    NRF24_REG_RF_SETUP    = 0x06,
    NRF24_REG_STATUS      = 0x07,

    NRF24_REG_TX_ADDR     = 0x10,

    NRF24_REG_RX_PW_P0    = 0x11,
    NRF24_REG_RX_PW_P1    = 0x12,
    NRF24_REG_RX_PW_P2    = 0x13,
    NRF24_REG_RX_PW_P3    = 0x14,
    NRF24_REG_RX_PW_P4    = 0x15,
    NRF24_REG_RX_PW_P5    = 0x16,

    NRF24_REG_FIFO_STATUS = 0x17,

    NRF24_REG_DYNPD       = 0x1C,
    NRF24_REG_FEATURE     = 0x1D,
} Nrf24RegsAddr_t;

// === CONFIGURATION ===
// === 0x00 CONFIG  ===

typedef enum {
    NRF24_PRIM_TRX = 1,
} Nrf24ConfigBits_t;

#define NRF24_CONFIG_PRIM_TRX    (0U << 0) // 0 TX,  1 RX
#define NRF24_CONFIG_PWR_UP      (1U << 1) // 0 Off, 1 On
#define NRF24_CONFIG_CRCO        (1U << 2) // 0 1B,  1 2B
#define NRF24_CONFIG_EN_CRC      (1U << 3) // 0 Off, 1 On
#define NRF24_CONFIG_MASK_MAX_RT (1U << 4) // 0 On,  1 Off - mask for exceeding send attempts
#define NRF24_CONFIG_MASK_TX_DS  (1U << 5) // 0 On,  1 Off - mask successful submission
#define NRF24_CONFIG_MASK_RX_DR  (1U << 6) // 0 On,  1 Off - mask data reception on IRQ

#define NRF24_CONFIG_POWER_UP  (NRF24_CONFIG_PRIM_TRX    | NRF24_CONFIG_PWR_UP |\
                                NRF24_CONFIG_CRCO        | NRF24_CONFIG_EN_CRC |\
                                NRF24_CONFIG_MASK_MAX_RT |\
                                NRF24_CONFIG_MASK_TX_DS  |\
                                NRF24_CONFIG_MASK_RX_DR )                             
#define NRF24_CONFIG_POWER_DOWN 0x00

// === 0x01 EN_AA ===

#define NRF24_ENAA_P0 (0U << 0) //0 Off, 1 On
#define NRF24_ENAA_P1 (0U << 1) //0 Off, 1 On
#define NRF24_ENAA_P2 (0U << 2) //0 Off, 1 On
#define NRF24_ENAA_P3 (0U << 3) //0 Off, 1 On
#define NRF24_ENAA_P4 (0U << 4) //0 Off, 1 On
#define NRF24_ENAA_P5 (0U << 5) //0 Off, 1 On

#define NRF24_ENAA (NRF24_ENAA_P0 | NRF24_ENAA_P1 | NRF24_ENAA_P2 |\
                    NRF24_ENAA_P3 | NRF24_ENAA_P4 | NRF24_ENAA_P5)

// == 0x02 EN_RXADDR ===

#define NRF24_ERX_P0 (0U << 0) //0 Off, 1 On
#define NRF24_ERX_P1 (0U << 1) //0 Off, 1 On
#define NRF24_ERX_P2 (0U << 2) //0 Off, 1 On
#define NRF24_ERX_P3 (0U << 3) //0 Off, 1 On
#define NRF24_ERX_P4 (0U << 4) //0 Off, 1 On
#define NRF24_ERX_P5 (0U << 5) //0 Off, 1 On

#define NRF24_ERX (NRF24_ERX_P0 | NRF24_ERX_P1 | NRF24_ERX_P2\
                   NRF24_ERX_P3 | NRF24_ERX_P4 | NRF24_ERX_P5)

// === 0x03 SETUP_AW === 

#define NRF24_AW_3BYTES 0x01U
#define NRF24_AW_4BYTES 0x02U
#define NRF24_AW_5BYTES 0x03U

// === 0x04 SETUP_RETR ===

#define NRF24_RETR_ARC 0x00U //0-3b auto retransmit count
#define NRF24_RETR_ARD 0x00U //4-7b auto retransmit delay

#define NRF24_RETR (NRF24_RETR_ARC | NRF24_RETR_ARD)

// === 0x05 RF_CH ===

#define NRF24_RF_CH 100U //0-125 (2400 + n = frequency)

// === 0x06 RF_SETUP ===
typedef enum {
    NRF24_PWR_MINUS_18DBM = (0x00U << 1),
    NRF24_PWR_MINUS_12DBM = (0x01U << 1),
    NRF24_PWR_MINUS_6DBM  = (0x02U << 1),
    NRF24_PWR_0DBM        = (0x03U << 1),
} Nrf24RfPwr_t;

typedef enum {
    NRF24_DR_1MBPS   = (0U << 3),
    NRF24_DR_2MBPS   = (1U << 3),
    NRF24_DR_250KBPS = (1U << 5),
} Nrf24RfDr_t;

#define NRF24_RF_PWR NRF24_PWR_0DBM
#define NRF24_RF_DR NRF24_DR_1MBPS
#define NRF24_PLL_LOCK (0U << 4)

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

// === MACRO ===

#define NRF24_DELAY_US(us) do {                                                     \
    uint32_t count = ((us) * (SystemCoreClock / 1000000UL)) / 4; \
    while(count--) {                                                                \
        __NOP();                                                                    \
    }                                                                               \
} while(0)

static uint8_t NRF24_SPI_WriteByte(SPI_TypeDef *SPIx, uint8_t data) {
    if (LL_SPI_IsActiveFlag_OVR(SPIx)) {
        LL_SPI_ReceiveData8(SPIx);
    }
    
    if(!WAIT_FLAG(LL_SPI_IsActiveFlag_TXE(SPIx), SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_TXRX);
    }
    LL_SPI_TransmitData8(SPIx, data);
    
    if(!WAIT_FLAG(LL_SPI_IsActiveFlag_RXNE(SPIx), SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_TXRX);
    }
    return LL_SPI_ReceiveData8(SPIx);
}

static uint8_t NRF24_AccessReg(SPI_TypeDef *SPIx, uint8_t cmd_type, 
                               Nrf24RegsAddr_t reg_addr, uint8_t data) {
    uint8_t val;
    LL_GPIO_ResetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
    
    NRF24_SPI_WriteByte(SPIx, cmd_type | (reg_addr & 0x1F));
    val = NRF24_SPI_WriteByte(SPIx, data);
    
    if (!WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_BSY);
    }
    LL_GPIO_SetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
    return val; 
}

static void NRF24_WriteByteBuf(SPI_TypeDef *SPIx, uint8_t cmd, uint8_t *buf, uint8_t buf_size) {
    LL_GPIO_ResetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
    NRF24_SPI_WriteByte(SPIx, cmd);
    for(uint8_t i = 0; i < buf_size; i++) {
        NRF24_SPI_WriteByte(SPIx, buf[i]);
    }
    if(!WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_BSY);
    }
    LL_GPIO_SetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
}

void NRF24_Init(SPI_TypeDef *SPIx){
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_EN_AA, 0x00);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_SETUP_RETR, 0x00);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_SETUP_AW, NRF24_AW_5BYTES);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RF_CH, 100);

    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RF_SETUP, NRF24_DR_1MBPS | NRF24_PWR_0DBM);

    uint8_t addr[5] = {0x9c, 0x96, 0xf1, 0x1f, 0x5e}; //receiver address
    NRF24_WriteByteBuf(SPIx, NRF24_CMD_W_REGISTER | NRF24_REG_TX_ADDR, addr, sizeof(addr));
    
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_STATUS, NRF24_STATUS_CLEAR_ALL);

    uint8_t check_aw = NRF24_AccessReg(SPIx, NRF24_CMD_R_REGISTER, NRF24_REG_SETUP_AW, DUMMY_BYTE);
    if (check_aw != NRF24_AW_5BYTES) BSP_ErrorSet(ERR_NRF_ERROR);
}

bool NRF24_TransmitData(SPI_TypeDef *SPIx, NRF24_Data_t *nrf24_data, uint8_t nrf24_data_size) {
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_CONFIG, NRF24_CONFIG_POWER_UP);
    BSP_LowPowerDelay(NRF24_WAKEUP_DELAY_MS);

    NRF24_WriteByteBuf(SPIx, NRF24_CMD_W_TX_PAYLOAD, (uint8_t*)nrf24_data, nrf24_data_size);
    LL_GPIO_SetOutputPin(NRF24_CE_PORT, NRF24_CE_PIN);
    NRF24_DELAY_US(NRF24_CE_DELAY_US);
    LL_GPIO_ResetOutputPin(NRF24_CE_PORT, NRF24_CE_PIN);

    if(!WAIT_FLAG(NRF24_AccessReg(SPIx, NRF24_CMD_R_REGISTER, NRF24_REG_STATUS, DUMMY_BYTE) & NRF24_STATUS_TX_DS_MASK, SPI_TIMEOUT_MS)) {
            NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_STATUS, NRF24_STATUS_CLEAR_ALL);
            BSP_ErrorSet(ERR_NRF_ERROR);
            return false;
    }

    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_STATUS, NRF24_STATUS_TX_DS_MASK);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_CONFIG, NRF24_CONFIG_POWER_DOWN);
    return true;
}