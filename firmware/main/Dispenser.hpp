#pragma once

#include <mutex>
#include <memory>
#include "infraredsensor.hpp"
#include "Nema17.hpp"
#include "debug.hpp"
#include "memory/pers_mem.hpp"

class Dispenser
{
public:
    inline static Dispenser& GetInstance()
    {
        static Dispenser instance;
        return instance;
    }
private:
    Dispenser():
        m_sensor(nullptr),
        m_motor(nullptr),
        m_calibrationPortionPerTurn(25.0f),
        m_PortionWeight(5.0f),
        m_tankLevel(0.0f)
    {}
    Dispenser(const Dispenser&) = delete;
    Dispenser& operator=(const Dispenser&) = delete;
    ~Dispenser(){}
public:
    void Init();
    inline float GetPortionPerTurnCalibration()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_calibrationPortionPerTurn;
    }
    inline void SetPortionPerTurnCalibration(float value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_calibrationPortionPerTurn = std::clamp(value, 5.0f, 80.0f);
        Memory::GetMemory().Set<float>(DATA_PORTION_PER_TURN, m_calibrationPortionPerTurn);
    }
    inline float GetPortionWeight()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_PortionWeight;
    }
    inline void SetPortionWeight(float value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_PortionWeight = std::clamp(value, 1.0f, 100.0f);
        Memory::GetMemory().Set<float>(DATA_PORTION_PER_TURN, m_calibrationPortionPerTurn);
    }
    inline uint32_t GetMotorSpeed()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<uint32_t>(m_motor->GetSpeedMode());
    }
    inline void SetMotorSpeed(uint32_t value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        eMotorSpeed speed = std::clamp(static_cast<eMotorSpeed>(value), eMotorSpeed::slow, eMotorSpeed::fast);
        m_motor->SetSpeed(speed);
        Memory::GetMemory().Set<uint32_t>(DATA_MOTOR_SPEED, static_cast<uint32_t>(speed));
    };

    inline void DispenseFoodContinue(bool toggle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_motor->Start(0/*continous*/);
    }
    inline void DispenseFoodPortion()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        float turns = m_PortionWeight / m_calibrationPortionPerTurn;
        DebugLogger::GetInstance().print(DEBUG_DISPENSER, DEBUG_INFO, "food portion computed : %.1f turn(s)", turns);
        m_motor->Start(turns);
    }

    void Routine();
    void ZigbeeReport();
private:
    float _GetTankLevel();
    void _DisplayRemainingMotorTour();
private:
    std::mutex m_mutex;
    std::unique_ptr<InfraredSensor> m_sensor;
    std::unique_ptr<Nema17> m_motor;

    // zigbee exposed data
    float m_calibrationPortionPerTurn;
    float m_PortionWeight;
    float m_tankLevel;
};
