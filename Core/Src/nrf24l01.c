#include "nrf24l01.h"
#include "nrf24l01_cfg.h"

#define NRF24_WAKEUP_DELAY_MS   3
#define NRF24_CE_DELAY_US       20
#define NRF24_SPI_TIMEOUT_MS    5

#define NRF24_SPI_DUMMY_BYTE    0xFFU
#define NRF24_REG_NONE          0x00U
#define NRF24_CMD_NOP           0xFFU

#define NRF24_REG_ADDR_MASK     0x1FU

extern uint32_t SystemCoreClock;

typedef enum {
    NRF24_REG_CONFIG      = 0x00,
    NRF24_REG_EN_AA       = 0x01,
    NRF24_REG_EN_RXADDR   = 0x02,
    NRF24_REG_SETUP_AW    = 0x03,
    NRF24_REG_SETUP_RETR  = 0x04,
    NRF24_REG_RF_CH       = 0x05,
    NRF24_REG_RF_SETUP    = 0x06,
    NRF24_REG_STATUS      = 0x07,

    NRF24_REG_RX_ADDR_P0  = 0x0A,
    NRF24_REG_RX_ADDR_P1  = 0x0B,
    NRF24_REG_RX_ADDR_P2  = 0x0C,
    NRF24_REG_RX_ADDR_P3  = 0x0D,
    NRF24_REG_RX_ADDR_P4  = 0x0E,
    NRF24_REG_RX_ADDR_P5  = 0x0F,

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

// === COMMANDS ===

#define NRF24_CMD_R_REGISTER    0x00U
#define NRF24_CMD_W_REGISTER    0x20U
#define NRF24_CMD_R_RX_PAYLOAD  0x61U
#define NRF24_CMD_W_TX_PAYLOAD  0xA0U
#define NRF24_CMD_FLUSH_TX      0xE1U
#define NRF24_CMD_FLUSH_RX      0xE2U
#define NRF24_CMD_REUSE_TX_PL   0xE3U

#define NRF24_CMD_R_RX_PL_WID   0x60U
#define NRF24_CMD_W_ACK_PAYLOAD 0xA8U // Require EN_ACK_PAY in 0x1D FEATURE
#define NRF24_CMD_W_TX_PAYLOAD_NOACK 0xB0U // Require EN_DYN_ACK in 0x1D FEATURE
#define NRF24_CMD_NOP           0xFFU

static uint8_t NRF24_SPI_TransmitByte(SPI_TypeDef *SPIx, uint8_t data) {
    if (LL_SPI_IsActiveFlag_OVR(SPIx)) LL_SPI_ReceiveData8(SPIx);
    
    if(!WAIT_FLAG(LL_SPI_IsActiveFlag_TXE(SPIx), NRF24_SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_TXRX);
    }
    LL_SPI_TransmitData8(SPIx, data);
    
    if(!WAIT_FLAG(LL_SPI_IsActiveFlag_RXNE(SPIx), NRF24_SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_TXRX);
    }
    return LL_SPI_ReceiveData8(SPIx);
}

static uint8_t NRF24_TransmitCmd(SPI_TypeDef *SPIx, uint8_t cmd) {
    LL_GPIO_ResetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);

    uint8_t status = NRF24_SPI_TransmitByte(SPIx, cmd);

    if (!WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), NRF24_SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_BSY);
    }
    LL_GPIO_SetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
    return status;
}

static uint8_t NRF24_AccessReg(SPI_TypeDef *SPIx, uint8_t cmd_type, 
                               Nrf24RegsAddr_t reg_addr, uint8_t data) {
    uint8_t val;
    LL_GPIO_ResetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
    
    NRF24_SPI_TransmitByte(SPIx, cmd_type | (reg_addr & NRF24_REG_ADDR_MASK));
    val = NRF24_SPI_TransmitByte(SPIx, data);
    
    if (!WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), NRF24_SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_BSY);
    }
    LL_GPIO_SetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
    return val; 
}

static void NRF24_TransmitBuffer(SPI_TypeDef *SPIx, uint8_t cmd,
                                 Nrf24RegsAddr_t reg_addr, const uint8_t *buf, 
                                 uint8_t buf_size){
    LL_GPIO_ResetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
    NRF24_SPI_TransmitByte(SPIx, cmd | (reg_addr & NRF24_REG_ADDR_MASK));
    for(uint8_t i = 0; i < buf_size; i++) {
        NRF24_SPI_TransmitByte(SPIx, buf[i]);
    }

    if(!WAIT_FLAG(!LL_SPI_IsActiveFlag_BSY(SPIx), NRF24_SPI_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_NRF_BSY);
    }
    LL_GPIO_SetOutputPin(NRF24_CSN_PORT, NRF24_CSN_PIN);
}

void NRF24_Init(SPI_TypeDef *SPIx){
    uint8_t status = NRF24_TransmitCmd(SPIx, NRF24_CMD_NOP);
    if (status == 0x00 || status == 0xFF) {
        BSP_ErrorSet(ERR_NRF_NOT_FOUND);
        return;
    }
    
    NRF24_TransmitCmd(SPIx, NRF24_CMD_FLUSH_RX);
    NRF24_TransmitCmd(SPIx, NRF24_CMD_FLUSH_TX);

    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_EN_AA, NRF24_ENAA);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_EN_RXADDR, NRF24_ERX);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_SETUP_AW, NRF24_AW);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_SETUP_RETR, NRF24_RETR);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RF_CH, NRF24_RF_CH);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RF_SETUP, NRF24_RF);

    NRF24_TransmitBuffer(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_ADDR_P0, NRF24_RX_ADDR_P0, sizeof(NRF24_RX_ADDR_P0));
    NRF24_TransmitBuffer(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_ADDR_P1, NRF24_RX_ADDR_P1, sizeof(NRF24_RX_ADDR_P1));
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_ADDR_P2, NRF24_RX_ADDR_P2);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_ADDR_P3, NRF24_RX_ADDR_P3);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_ADDR_P4, NRF24_RX_ADDR_P4);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_ADDR_P5, NRF24_RX_ADDR_P5);
    
    NRF24_TransmitBuffer(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_TX_ADDR, NRF24_TX_ADDR, sizeof(NRF24_TX_ADDR));

    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_PW_P0, NRF24_RX_PW_P0);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_PW_P1, NRF24_RX_PW_P1);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_PW_P2, NRF24_RX_PW_P2);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_PW_P3, NRF24_RX_PW_P3);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_PW_P4, NRF24_RX_PW_P4);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_RX_PW_P5, NRF24_RX_PW_P5);

    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_DYNPD, NRF24_DYNPD);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_FEATURE, NRF24_FEAT);

    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_STATUS, NRF24_STATUS_CLEAR_FLAGS);
}

bool NRF24_TransmitData(SPI_TypeDef *SPIx, NRF24_Data_t *nrf24_data, uint8_t nrf24_data_size) {
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_CONFIG, NRF24_CONFIG_FULL);
    BSP_LowPowerDelay(NRF24_WAKEUP_DELAY_MS);
    
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_STATUS, NRF24_STATUS_CLEAR_FLAGS);

    NRF24_TransmitBuffer(SPIx, NRF24_CMD_W_TX_PAYLOAD_NOACK, NRF24_REG_NONE, 
                        (uint8_t*)nrf24_data, nrf24_data_size);

    LL_GPIO_SetOutputPin(NRF24_CE_PORT, NRF24_CE_PIN);
    BSP_DelayUS(NRF24_CE_DELAY_US);
    LL_GPIO_ResetOutputPin(NRF24_CE_PORT, NRF24_CE_PIN);

    if(!WAIT_FLAG(NRF24_TransmitCmd(SPIx, NRF24_CMD_NOP) & NRF24_STATUS_TX_DS_MASK, NRF24_SPI_TIMEOUT_MS)) {
        NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_STATUS, NRF24_STATUS_CLEAR_FLAGS);
        NRF24_TransmitCmd(SPIx, NRF24_CMD_FLUSH_TX);
        BSP_ErrorSet(ERR_NRF_NOT_FOUND);
        return false;
    }

    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_STATUS, NRF24_STATUS_TX_DS_MASK);
    NRF24_AccessReg(SPIx, NRF24_CMD_W_REGISTER, NRF24_REG_CONFIG, NRF24_CONFIG_POWER_DOWN);
    return true;
}