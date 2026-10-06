#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

extern "C" {
    #include "VL53L1X_api.h"
    #include "vl53l1_platform.h"
}

#define I2C_DEVICE_ADDRESS  0x52

#define I2C_REG_ADD_MODEL_ID        0x010F
#define I2C_REG_ADD_MODULE_TYPE     0x0110
#define I2C_REG_ADD_MASK_REVISION   0x0111

struct I2C_Desc {
    i2c_port_num_t port;
    gpio_num_t sda;
    gpio_num_t scl;
};

class InfraredSensor
{
public:
    InfraredSensor(I2C_Desc desc):
        m_i2cDesc(desc),
        m_i2cAddress(I2C_DEVICE_ADDRESS),
        m_devHandler(nullptr),
        m_devId(0xFFFF)
    {}
    ~InfraredSensor(){}

public:
    void Init();
    uint16_t GetDistance();

private:
    I2C_Desc m_i2cDesc;
    uint8_t m_i2cAddress;
    i2c_master_dev_handle_t m_devHandler;
    uint16_t m_devId;
};