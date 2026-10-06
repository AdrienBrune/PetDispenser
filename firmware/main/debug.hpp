#pragma once

#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/uart.h"

/* Debug enable per component (empty string = disabled) */
#define DEBUG_GENERIC   "generic"
#define DEBUG_TASK      "task"
#define DEBUG_BUTTONS   "buttons"
#define DEBUG_DISPENSER "dispenser"
#define DEBUG_ZIGBEE    "zigbee"
#define DEBUG_EEPROM    "flash"
#define DEBUG_SENSOR    "sensor"
#define DEBUG_MOTOR     "motor"

/* Debug levels */
#define DEBUG_INFO      1
#define DEBUG_WARNING   2
#define DEBUG_ERROR     4
#define DEBUG_FILTER    (DEBUG_INFO | DEBUG_WARNING | DEBUG_ERROR)

class DebugLogger
{
public:
    inline static DebugLogger& GetInstance()
    {
        static DebugLogger instance;
        return instance;
    }

    inline void print(std::string componant, uint8_t filter, const char* format, ...)
    {
        if(componant == "") // Debug disabled
            return;
        
        if(!(DEBUG_FILTER & filter)) // Filter messages
            return;

        if(xSemaphoreTake(mSemaphore, portMAX_DELAY))
        {
            std::string filter_str;
            switch(filter)
            {
                case DEBUG_INFO: filter_str = "INFO"; break;
                case DEBUG_WARNING: filter_str = "WARN"; break;
                case DEBUG_ERROR: filter_str = "ERROR"; break;
                default: filter_str = "????"; break;
            }

            va_list args;
            va_start(args, format);
            char buffer[256];
            
            int len = snprintf(buffer, sizeof(buffer), "[%s][%s] : ", filter_str.c_str(), componant.c_str());            
            vsnprintf(buffer + len, sizeof(buffer) - len, format, args);
            va_end(args);

            printf("%s\n", buffer);

            xSemaphoreGive(mSemaphore);
        }
    }

private:
    DebugLogger()
    {
        mSemaphore = xSemaphoreCreateMutex();
    }

    ~DebugLogger()
    {
        if (mSemaphore)
        {
            vSemaphoreDelete(mSemaphore);
            mSemaphore = nullptr;
        }
    }

    DebugLogger(const DebugLogger&) = delete;
    DebugLogger& operator=(const DebugLogger&) = delete;

    SemaphoreHandle_t mSemaphore;
};
