#ifndef NRF24L01_CFG_H
#define NRF24L01_CFG_H

#include "stdint.h"

typedef enum {
    NRF24_PWR_MINUS_18DBM = 0x00U,
    NRF24_PWR_MINUS_12DBM = 0x01U,
    NRF24_PWR_MINUS_6DBM  = 0x02U,
    NRF24_PWR_0DBM        = 0x03U,
} Nrf24RfPwr_t;

typedef enum {
    NRF24_DR_1MBPS   = 0x00U,
    NRF24_DR_2MBPS   = 0x01U,
    NRF24_DR_250KBPS = 0x04U,
} Nrf24RfDr_t;

typedef enum {
    NRF24_AW_3BYTES = 0x01U,
    NRF24_AW_4BYTES = 0x02U,
    NRF24_AW_5BYTES = 0x03U,
} Nrf24AwBytes_t;

typedef enum {
    NRF24_STATUS_TX_FULL_MASK  = (1U << 0), 
    NRF24_STATUS_RX_P_NO_MASK  = (0x07U << 1),
    NRF24_STATUS_MAX_RT_MASK   = (1U << 4), 
    NRF24_STATUS_TX_DS_MASK    = (1U << 5), 
    NRF24_STATUS_RX_DR_MASK    = (1U << 6), 
    NRF24_STATUS_RX_EMPTY_MASK = (1U << 7),
} Nrf24Status_t;

// === CONFIGURATION ===
// === 0x00 CONFIG  ===

#define NRF24_CONFIG_PRIM_TRX    (0U << 0) // 0 TX,  1 RX
#define NRF24_CONFIG_PWR_UP      (1U << 1) // 0 Off, 1 On
#define NRF24_CONFIG_CRCO        (1U << 2) // 0 1B,  1 2B
#define NRF24_CONFIG_EN_CRC      (1U << 3) // 0 Off, 1 On
#define NRF24_CONFIG_MASK_MAX_RT (1U << 4) // 0 On,  1 Off - mask for exceeding send attempts
#define NRF24_CONFIG_MASK_TX_DS  (1U << 5) // 0 On,  1 Off - mask successful submission
#define NRF24_CONFIG_MASK_RX_DR  (1U << 6) // 0 On,  1 Off - mask data reception on IRQ

#define NRF24_CONFIG_FULL (NRF24_CONFIG_PRIM_TRX    | NRF24_CONFIG_PWR_UP |\
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

#define NRF24_ERX (NRF24_ERX_P0 | NRF24_ERX_P1 | NRF24_ERX_P2 |\
                   NRF24_ERX_P3 | NRF24_ERX_P4 | NRF24_ERX_P5)

// === 0x03 SETUP_AW === 

#define NRF24_AW NRF24_AW_5BYTES

// === 0x04 SETUP_RETR ===

#define NRF24_RETR_ARC 0x00U //0-3b auto retransmit count
#define NRF24_RETR_ARD 0x00U //4-7b auto retransmit delay

#define NRF24_RETR (NRF24_RETR_ARC | NRF24_RETR_ARD)

// === 0x05 RF_CH ===

#define NRF24_RF_CH 100U //0-125 (2400 + n = frequency)

// === 0x06 RF_SETUP ===

#define NRF24_RF_PWR    (NRF24_PWR_0DBM << 1)
#define NRF24_RF_DR     (NRF24_DR_1MBPS << 3)
#define NRF24_PLL_LOCK  (0U << 4) //0 Off, 1 On
#define NRF24_CONT_WAVE (0U << 7) //0 Off, 1 On

#define NRF24_RF (NRF24_RF_PWR   | NRF24_RF_DR |\
                  NRF24_PLL_LOCK | NRF24_CONT_WAVE)

// === 0x07 STATUS  ===

#define NRF24_STATUS_CLEAR_FLAGS (NRF24_STATUS_MAX_RT_MASK |\
                                  NRF24_STATUS_TX_DS_MASK  |\
                                  NRF24_STATUS_RX_DR_MASK)

// === 0x0A-0x0F RX_ADDR ===
#define NRF24_ADDR_WIDTH (NRF24_AW + 2U)

static const uint8_t NRF24_RX_ADDR_P0[NRF24_ADDR_WIDTH] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
static const uint8_t NRF24_RX_ADDR_P1[NRF24_ADDR_WIDTH] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
#define NRF24_RX_ADDR_P2 0xC3U //2-4B from NRF24_RX_ADDR_P1
#define NRF24_RX_ADDR_P3 0xC4U //2-4B from NRF24_RX_ADDR_P1
#define NRF24_RX_ADDR_P4 0xC5U //2-4B from NRF24_RX_ADDR_P1
#define NRF24_RX_ADDR_P5 0xC6U //2-4B from NRF24_RX_ADDR_P1

// === 0x10 TX_ADDR ===

static const uint8_t NRF24_TX_ADDR[NRF24_ADDR_WIDTH] = {0x9C, 0x96, 0xF1, 0x1F, 0x5E};

// === 0x11-0x16 RX_PW ===

#define NRF24_RX_PW_P0 0U //0 Off, 1-32B
#define NRF24_RX_PW_P1 0U //0 Off, 1-32B
#define NRF24_RX_PW_P2 0U //0 Off, 1-32B
#define NRF24_RX_PW_P3 0U //0 Off, 1-32B
#define NRF24_RX_PW_P4 0U //0 Off, 1-32B
#define NRF24_RX_PW_P5 0U //0 Off, 1-32B

// === 0x17 FIFO_STATUS ===

#define NRF24_FIFO_RX_EMPTY_MASK_READ (1U << 0)
#define NRF24_FIFO_RX_FULL_MASK_READ  (1U << 1)
#define NRF24_FIFO_TX_EMPTY_MASK_READ (1U << 4)
#define NRF24_FIFO_TX_FULL_MASK_READ  (1U << 5) 
#define NRF24_FIFO_TX_REUSE_MASK_READ (1U << 6) 

// === 0x1C DYNPD ===
// required EN_DPL in 0x1D FEATURE
#define NRF24_DPL_P0 (0U << 0) //0 Off, 1 On
#define NRF24_DPL_P1 (0U << 1) //0 Off, 1 On
#define NRF24_DPL_P2 (0U << 2) //0 Off, 1 On
#define NRF24_DPL_P3 (0U << 3) //0 Off, 1 On
#define NRF24_DPL_P4 (0U << 4) //0 Off, 1 On
#define NRF24_DPL_P5 (0U << 5) //0 Off, 1 On

#define NRF24_DYNPD (NRF24_DPL_P0 | NRF24_DPL_P1 | NRF24_DPL_P2 |\
                     NRF24_DPL_P3 | NRF24_DPL_P4 | NRF24_DPL_P5 )

// === 0x1D FEATURE ===

#define NRF24_FEAT_EN_DYN_ACK (1U << 0) //0 Off, 1 On
#define NRF24_FEAT_EN_ACK_PAY (0U << 1) //0 Off, 1 On
#define NRF24_FEAT_EN_DPL     (0U << 2) //0 Off, 1 On 

#define NRF24_FEAT (NRF24_FEAT_EN_DYN_ACK | NRF24_FEAT_EN_ACK_PAY | NRF24_FEAT_EN_DPL)

#endif