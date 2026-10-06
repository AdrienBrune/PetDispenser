#include "infraredsensor.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "debug.hpp"

#define CHECK_VL53L1X_ERROR(function) \
    do { \
        VL53L1X_ERROR ret = function; \
        if (ret != VL53L1X_ERROR_NONE) \
        { \
            DebugLogger::GetInstance().print( \
                DEBUG_SENSOR, \
                DEBUG_ERROR, \
                "Error in %s: %d", \
                #function, \
                ret \
            ); \
        } \
    } while(0)

void InfraredSensor::Init()
{
    i2c_master_bus_config_t busConfig = {
        .i2c_port = m_i2cDesc.port,
        .sda_io_num = m_i2cDesc.sda,
        .scl_io_num = m_i2cDesc.scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true
        }
    };

    i2c_master_bus_handle_t busHandle;
    if (i2c_new_master_bus(&busConfig, &busHandle) != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_ERROR, "I2C bus creation failure");
        return;
    }
    if (i2c_master_probe(busHandle, 0x29, 100) != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_ERROR, "Aucun périphérique à l'adresse 0x29");
    }

    i2c_device_config_t devConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = static_cast<uint16_t>(m_i2cAddress >> 1), // (7bits)
        .scl_speed_hz = 400000,
    };

    if (i2c_master_bus_add_device(busHandle, &devConfig, &m_devHandler) != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_ERROR, "I2C add device failure");
        m_devHandler = nullptr;
        return;
    }

    // provide handler to ST driver
    m_devId = VL53L1_RegisterDevice(m_devHandler);
    if (m_devId == 0xFFFF)
    {
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_ERROR, "no more I2C instances can be created");
        return;
    }
    DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_INFO, "device id registered: %d", m_devId);

    // wait for boot
    uint8_t isBooted = 0;
    int timeout = 50; // Max 500ms
    while (!isBooted && timeout > 0)
    {
        CHECK_VL53L1X_ERROR(VL53L1X_BootState(m_devId, &isBooted));
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout--;
    }

    if (!isBooted) {
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_ERROR, "VL53L1X Boot failed");
        m_devHandler = nullptr;
        return;
    }

    // Firmware init
    CHECK_VL53L1X_ERROR(VL53L1X_SensorInit(m_devId));
    CHECK_VL53L1X_ERROR(VL53L1X_SetDistanceMode(m_devId, 2)); // long distance (4m)
    CHECK_VL53L1X_ERROR(VL53L1X_SetTimingBudgetInMs(m_devId, 50)); // Budget 50ms

    // ROI configuration 16x16 -> 4x4
    CHECK_VL53L1X_ERROR(VL53L1X_SetROI(m_devId, 8, 8));
    CHECK_VL53L1X_ERROR(VL53L1X_SetROICenter(m_devId, 199));

    // resume measurements
    CHECK_VL53L1X_ERROR(VL53L1X_StartRanging(m_devId));

    DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_INFO, "device initialized");
}

uint16_t InfraredSensor::GetDistance()
{
    if (!m_devHandler)
    {
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_WARNING, "device has not been initialized");
        return 0;
    }

    uint8_t dataReady = 0;
    uint16_t distance_mm = 0;
    int timeout = 8;

    while (!dataReady && timeout > 0)
    {
        VL53L1X_CheckForDataReady(m_devId, &dataReady);
        if (!dataReady)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        timeout--;
    }

    if (dataReady)
    {
        VL53L1X_GetDistance(m_devId, &distance_mm);
        VL53L1X_ClearInterrupt(m_devId);
        uint16_t distance_cm = (distance_mm + 5) / 10;
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_INFO, "Distance measured: %d cm", distance_cm);
        return distance_cm;
    }
    else
    {
        DebugLogger::GetInstance().print(DEBUG_SENSOR, DEBUG_ERROR, "Distance data not ready (timeout)");
        return 0;
    }
}