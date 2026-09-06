#include "i2c_utils.h"

#define I2C_TIMEOUT_MS 5

extern volatile uint32_t system_ticks;

bool I2C_TransmitData(I2C_TypeDef *I2Cx, uint8_t slave_addr, uint8_t *cmd_data, uint8_t cmd_size) {
    if (!WAIT_FLAG(!LL_I2C_IsActiveFlag_BUSY(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_BUSY);
        return false;
    }
    LL_I2C_HandleTransfer(I2Cx, slave_addr, 
                        LL_I2C_ADDRSLAVE_7BIT, cmd_size, 
                        LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    for (uint8_t i = 0; i < cmd_size; i++) {
        if(!WAIT_FLAG(LL_I2C_IsActiveFlag_TXIS(I2Cx), I2C_TIMEOUT_MS)) {
            BSP_ErrorSet(ERR_I2C_TXRX);
            return false;
        }
        LL_I2C_TransmitData8(I2Cx, cmd_data[i]);
    }
    if (!WAIT_FLAG(LL_I2C_IsActiveFlag_STOP(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_STOP);
        return false;
    }
    if (!WAIT_FLAG(!LL_I2C_IsActiveFlag_BUSY(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_BUSY);
        return false;
    }
    return true;
}

bool I2C_ReceiveData(I2C_TypeDef *I2Cx, uint8_t slave_addr, 
                  uint8_t *data_buffer, uint8_t data_size) {
    if (!WAIT_FLAG(!LL_I2C_IsActiveFlag_BUSY(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_BUSY);
        return false;
    }
    LL_I2C_HandleTransfer(I2Cx, slave_addr,
                          LL_I2C_ADDRSLAVE_7BIT, data_size, 
                          LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    for (uint8_t i = 0; i < data_size; i++) {
        if(!WAIT_FLAG(LL_I2C_IsActiveFlag_RXNE(I2Cx), I2C_TIMEOUT_MS)) {
            BSP_ErrorSet(ERR_I2C_TXRX);
            return false;
        }
        data_buffer[i] = LL_I2C_ReceiveData8(I2Cx);
    }
    if (!WAIT_FLAG(LL_I2C_IsActiveFlag_STOP(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_STOP);
        return false;
    }
    if (!WAIT_FLAG(!LL_I2C_IsActiveFlag_BUSY(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_BUSY);
        return false;
    }
    return true;
}

bool I2C_ReceiveRegsData(I2C_TypeDef *I2Cx, uint8_t slave_addr, 
    uint8_t reg_addr, uint8_t *data_buffer, uint8_t data_size) {
    if (!WAIT_FLAG(!LL_I2C_IsActiveFlag_BUSY(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_BUSY);
        return false;
    }
    //Transmit register address
    LL_I2C_HandleTransfer(I2Cx, slave_addr, 
                        LL_I2C_ADDRSLAVE_7BIT, 1, 
                        LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
    if(!WAIT_FLAG(LL_I2C_IsActiveFlag_TXIS(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_TXRX);
        return false;
    }
    LL_I2C_TransmitData8(I2Cx, reg_addr);
    if (!WAIT_FLAG(LL_I2C_IsActiveFlag_TC(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_STOP);
        return false;
    }

    //Receive register data
    LL_I2C_HandleTransfer(I2Cx, slave_addr,
                          LL_I2C_ADDRSLAVE_7BIT, data_size, 
                          LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    for (uint8_t i = 0; i < data_size; i++) {
        if(!WAIT_FLAG(LL_I2C_IsActiveFlag_RXNE(I2Cx), I2C_TIMEOUT_MS)) {
            BSP_ErrorSet(ERR_I2C_TXRX);
            return false;
        }
        data_buffer[i] = LL_I2C_ReceiveData8(I2Cx);
    }
    if (!WAIT_FLAG(LL_I2C_IsActiveFlag_STOP(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_STOP);
        return false;
    }
    if (!WAIT_FLAG(!LL_I2C_IsActiveFlag_BUSY(I2Cx), I2C_TIMEOUT_MS)) {
        BSP_ErrorSet(ERR_I2C_BUSY);
        return false;
    }
    return true;
}
