#include "vl53l1_platform.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define MAX_VL53L1X_SENSORS 4
static i2c_master_dev_handle_t s_i2c_handles[MAX_VL53L1X_SENSORS] = {NULL};

uint16_t VL53L1_RegisterDevice(i2c_master_dev_handle_t handle) {
    for (uint16_t i = 0; i < MAX_VL53L1X_SENSORS; i++)
    {
        if (s_i2c_handles[i] == NULL)
        {
            s_i2c_handles[i] = handle;
            return i; // return index to use
        }
    }
    return 0xFFFF;
}

static inline i2c_master_dev_handle_t get_handle(uint16_t dev) {
    if (dev < MAX_VL53L1X_SENSORS)
    {
        return s_i2c_handles[dev];
    }
    return NULL;
}

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count)
{
    i2c_master_dev_handle_t handle = get_handle(dev);
    if (!handle || count == 0 || pdata == NULL) return -1;

    uint8_t buffer[count + 2];
    buffer[0] = (uint8_t)(index >> 8);
    buffer[1] = (uint8_t)(index & 0xFF);
    memcpy(&buffer[2], pdata, count);

    esp_err_t err = i2c_master_transmit(handle, buffer, count + 2, -1);
    return (err == ESP_OK) ? 0 : -1;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count)
{
    i2c_master_dev_handle_t handle = get_handle(dev);
    if (!handle || count == 0 || pdata == NULL) return -1;

    uint8_t reg_buf[2] = {
        (uint8_t)(index >> 8),
        (uint8_t)(index & 0xFF)
    };

    esp_err_t err = i2c_master_transmit_receive(handle, reg_buf, 2, pdata, count, -1);
    return (err == ESP_OK) ? 0 : -1;
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data)
{
    return VL53L1_WriteMulti(dev, index, &data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data)
{
    uint8_t buffer[2] = {
        (uint8_t)(data >> 8),    // MSB
        (uint8_t)(data & 0xFF)   // LSB
    };
    return VL53L1_WriteMulti(dev, index, buffer, 2);
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data)
{
    uint8_t buffer[4] = {
        (uint8_t)(data >> 24),
        (uint8_t)(data >> 16),
        (uint8_t)(data >> 8),
        (uint8_t)(data & 0xFF)
    };
    return VL53L1_WriteMulti(dev, index, buffer, 4);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data)
{
    return VL53L1_ReadMulti(dev, index, data, 1);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data)
{
    uint8_t buffer[2];
    int8_t status = VL53L1_ReadMulti(dev, index, buffer, 2);
    if (status == 0) {
        *data = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
    }
    return status;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data)
{
    uint8_t buffer[4];
    int8_t status = VL53L1_ReadMulti(dev, index, buffer, 4);
    if (status == 0) {
        *data = ((uint32_t)buffer[0] << 24) |
                ((uint32_t)buffer[1] << 16) |
                ((uint32_t)buffer[2] << 8)  |
                 (uint32_t)buffer[3];
    }
    return status;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms)
{
    if (wait_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
    return 0;
}