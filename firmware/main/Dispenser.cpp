#include "Dispenser.hpp"
#include <atomic>
#include "zigbee.hpp"

#define ZIGBEE_REPORTING_DELAY  60000

void Dispenser::Init()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_motor = std::make_unique<Nema17>();
    m_motor->Init();

    m_sensor = std::make_unique<InfraredSensor>(InfraredSensor({I2C_NUM_0, GPIO_NUM_10, GPIO_NUM_11}));
    m_sensor->Init();
    
    uint32_t motorSpeed = std::clamp(Memory::GetMemory().Get<uint32_t>(DATA_MOTOR_SPEED), static_cast<uint32_t>(eMotorSpeed::slow), static_cast<uint32_t>(eMotorSpeed::fast));
    m_motor->SetSpeed(static_cast<eMotorSpeed>(motorSpeed));
    m_motor->SetSleepMode(Memory::GetMemory().Get<bool>(DATA_SLEEP_MODE));
    m_PortionWeight = Memory::GetMemory().Get<float>(DATA_PORTION_WEIGHT);
    m_calibrationPortionPerTurn = Memory::GetMemory().Get<float>(DATA_PORTION_PER_TURN);
    m_tankLevel = Memory::GetMemory().Get<float>(DATA_TANK_FILLING);
}

void Dispenser::Routine()
{
    uint32_t startMeasurementTime = pdTICKS_TO_MS(xTaskGetTickCount());

    while (1)
    {
        if (pdTICKS_TO_MS(xTaskGetTickCount()) - startMeasurementTime > ZIGBEE_REPORTING_DELAY)
        {
            startMeasurementTime = pdTICKS_TO_MS(xTaskGetTickCount());
            
            { // mutex context
                std::lock_guard<std::mutex> lock(m_mutex);

                m_tankLevel = _GetTankLevel();
                Memory::GetMemory().Set<float>(DATA_TANK_FILLING, m_tankLevel);
            }

            ZigbeeReport();
        }

        vTaskDelay(pdMS_TO_TICKS(300));      
    }
}

void Dispenser::ZigbeeReport()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "Start Zigbee report");
    DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "tank level       : %2.0f", m_tankLevel);
    DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "motor speed      : %d", static_cast<uint32_t>(m_motor->GetSpeedMode()));
    DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "portion per turn : %.1f", m_calibrationPortionPerTurn);
    DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "portion weight   : %.1f", m_PortionWeight);
    DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "sleep mode       : %s", m_motor->GetSleepMode() ? "true" : "false");

    updateTankFilling(ZB_EP_USER, m_tankLevel);
    updatePortionWeight(ZB_EP_USER, m_PortionWeight);
    updateMotorSpeed(ZB_EP_CONFIG, static_cast<uint32_t>(m_motor->GetSpeedMode()));
    updateTurnPerPortion(ZB_EP_CONFIG, m_calibrationPortionPerTurn);
    updateSleepMode(ZB_EP_CONFIG, m_motor->GetSleepMode());
}

float Dispenser::_GetTankLevel()
{
    const uint32_t DISTANCE_CM_TANK_100_PERCENT = 2;
    const uint32_t DISTANCE_CM_TANK_0_PERCENT = 16;

    float filling = 0.0f;
    uint16_t measurement = m_sensor->GetDistance();
    if (measurement)
    {
        uint16_t distance = std::clamp<uint16_t>(measurement, DISTANCE_CM_TANK_100_PERCENT, DISTANCE_CM_TANK_0_PERCENT);
        filling = (1.0f - (static_cast<float>(distance) / static_cast<float>(DISTANCE_CM_TANK_0_PERCENT))) * 100.0f;
        DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "tank info: %f% (d=%dcm)", filling, measurement);
    }
    return filling;
}
