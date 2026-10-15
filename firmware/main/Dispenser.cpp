#include "Dispenser.hpp"
#include <atomic>
#include "zigbee.hpp"

#define ZIGBEE_REPORTING_DELAY  60000

struct CalibrationTank {
    float distance_mm;
    float percentage;
};

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
    static const std::array<CalibrationTank, 2> calibrationTable = {{
        {0.0f, 100.0f},
        {110.0f, 0.0f}
    }};

    int16_t measurement = m_sensor->GetDistance();
    if (measurement < 0)
    {
        return m_tankLevel;
    }

    float distance_mm = static_cast<float>(measurement);
    float currentFilling = 0.0f;

    // linear interpolation
    if (distance_mm <= calibrationTable.front().distance_mm)
    {
        currentFilling = calibrationTable.front().percentage;
    }
    else if (distance_mm >= calibrationTable.back().distance_mm)
    {
        currentFilling = calibrationTable.back().percentage;
    }
    else
    {
        for (size_t i = 0; i < calibrationTable.size() - 1; ++i)
        {
            if (distance_mm >= calibrationTable[i].distance_mm && distance_mm <= calibrationTable[i+1].distance_mm)
            {
                float x0 = calibrationTable[i].distance_mm;
                float y0 = calibrationTable[i].percentage;
                float x1 = calibrationTable[i+1].distance_mm;
                float y1 = calibrationTable[i+1].percentage;
                currentFilling = y0 + (y1 - y0) * (distance_mm - x0) / (x1 - x0);
                break;
            }
        }
    }
    DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "tank level computed: %.2f%% (d=%.0fmm)", currentFilling, distance_mm);

    // threshold allowance to change value (5%)
    if (m_tankLevel < 0.0f || std::abs(currentFilling - m_tankLevel) >= 5.0f)
    {
        m_tankLevel = std::roundf(currentFilling);
        DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "tank info updated: %.2f%%", m_tankLevel);
    }

    return m_tankLevel;
}
