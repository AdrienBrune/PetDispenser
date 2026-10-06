#pragma once

#include <stdint.h>

#define STEP_PIN (gpio_num_t)1

#define MOTOR_1_8_DEG_CARACTERISTIC 200
#define MS1_MS2_MICROSTEPS_CONFIGURATION 64

enum eMotorSpeed
{
    slow,
    medium,
    fast
};

class Nema17
{
public:
    Nema17():
        m_speed(slow)
    {}
    ~Nema17(){}
public:
    void Init();
    void Start(float turns);
    void Stop();

    const eMotorSpeed& GetSpeedMode() const { return m_speed; }
    inline void SetSpeed(eMotorSpeed speed) { m_speed = speed; }

private:
    inline static uint32_t _GetMotorStepsPerTurn() { return MOTOR_1_8_DEG_CARACTERISTIC * MS1_MS2_MICROSTEPS_CONFIGURATION; }
    inline uint32_t _GetMotorStepDelayUs() { return (m_speed == slow) ? 315 : (m_speed == medium) ? 134 : (m_speed == fast) ? 67 : 134; }
    inline uint32_t _GetRpmSpeed() { return (uint32_t)(60000000 / ((float)_GetMotorStepDelayUs() * _GetMotorStepsPerTurn())); }
private:
    eMotorSpeed m_speed;
};