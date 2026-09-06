#include "aht20.h"

#define AHT20_I2C_ADDRESS        0x38 << 1

#define AHT20_MEASURE_DELAY_MS   90
#define AHT20_CALIBRATE_DELAY_MS 40

#define AHT20_STATUS_BUSY_BIT    0x80
#define AHT20_STATUS_CAL_BIT     0x08

bool AHT20_GetData(I2C_TypeDef *I2Cx, AHT20_Data_t *out_data) {
    //Clear flags
    LL_I2C_ClearFlag_NACK(I2Cx);
    LL_I2C_ClearFlag_BERR(I2Cx);

    //Transmit
    uint8_t measure_cmd_bytes[3] = {0xAC, 0x33, 0x00};
    if(!I2C_TransmitData(I2Cx, AHT20_I2C_ADDRESS, measure_cmd_bytes, 3)) return false;

    BSP_LowPowerDelay(AHT20_MEASURE_DELAY_MS);
    
    //Receive
    uint8_t receive_data_buffer[6];
    if(!I2C_ReceiveData(I2Cx, AHT20_I2C_ADDRESS, receive_data_buffer, 6)) return false;

    //Check errors
    if ((receive_data_buffer[0] & AHT20_STATUS_BUSY_BIT) != 0) return false;
    if ((receive_data_buffer[0] & AHT20_STATUS_CAL_BIT)  == 0) {
        uint8_t calibrate_cmd_bytes[] = {0xBE, 0x08, 0x00}; //Calibrate command
        if(!I2C_TransmitData(I2Cx, AHT20_I2C_ADDRESS, calibrate_cmd_bytes, 3)) return false;
        BSP_LowPowerDelay(AHT20_CALIBRATE_DELAY_MS);
        return false;
    }

    //Parsing
    uint32_t humidity = (((uint32_t)receive_data_buffer[1]) << 12) |
                        (((uint32_t)receive_data_buffer[2]) << 4) |
                        ((receive_data_buffer[3] & 0xF0) >> 4);
    uint32_t temperature = (((uint32_t)(receive_data_buffer[3] & 0x0F)) << 16) |
                           (((uint32_t)receive_data_buffer[4]) << 8) |
                           receive_data_buffer[5];

    //Calculation
    out_data->humidity = ((float)humidity * 100.0f) / 1048576.0f;
    out_data->temperature = (((float)temperature * 200.0f) / 1048576.0f) - 50.0f;

    return true;
}