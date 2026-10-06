#pragma once

#include <unordered_map>
#include "driver/gpio.h"

#define ZB_LED_PIN (gpio_num_t)1

class Led
{
public:
    inline static Led& GetInstance()
    {
        static Led instance;
        return instance;
    }
private:
    Led(){}
    Led(const Led&) = delete;
    Led& operator=(const Led&) = delete;
    ~Led(){}
public:
    inline void Init(gpio_num_t pin)
    {
        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        Set(pin, 0);
    }
    inline bool Get(gpio_num_t pin)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return gpio_get_level(pin) ? true : false;
    }
    inline void Set(gpio_num_t pin, bool state)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        gpio_set_level(pin, state ? 1 : 0);
    }
    inline void Pulse(gpio_num_t pin)
    {
        Set(pin, 0);
        Set(pin, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        Set(pin, 0);
    }
private:
    std::mutex m_mutex;
};