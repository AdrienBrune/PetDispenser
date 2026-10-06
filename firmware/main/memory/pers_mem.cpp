#include "memory/pers_mem.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "debug.hpp"

void Memory::_Save()
{
    std::lock_guard<std::mutex> lock(mMutex);

    DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_INFO, "Memory save requested");

    nvs_handle_t handle;
    esp_err_t err = nvs_open("configuration", NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_ERROR, "NVS open failed (%d)", err);
        return;
    }

    for (auto const& item : mDataMap)
    {
        esp_err_t set_err = ESP_OK;
        const char* key = item.first.c_str();
        switch(item.second.GetType())
        {
            case Data::eType::ebool:
            {
                uint8_t val = item.second.GetValue<bool>() ? 1 : 0;
                set_err = nvs_set_u8(handle, key, val);
                if (set_err == ESP_OK) {
                    DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_INFO, "%s save : %s", key, val ? "true" : "false");
                }
                break;
            }
            case Data::eType::euint:
            {
                uint32_t val = item.second.GetValue<uint32_t>();

                set_err = nvs_set_u32(handle, key, val);
                if (set_err == ESP_OK)
                {
                    DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_INFO, "%s save : %u", key, val);
                }
                break;
            }
            case Data::eType::efloat:
            {
                float val = item.second.GetValue<float>();
                set_err = nvs_set_blob(handle, key, &val, sizeof(float));
                if (set_err == ESP_OK)
                {
                    DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_INFO, "%s save : %.2f", key, val);
                }
                break;
            }
        }

        if (set_err != ESP_OK)
        {
            DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_ERROR, "NVS set failed for %s (%d)", key, set_err);
        }
    }

    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_ERROR, "NVS commit failed (%d)", err);
    }

    nvs_close(handle);
}

void Memory::Load()
{
    std::lock_guard<std::mutex> lock(mMutex);

    DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_INFO, "Memory load requested");

    nvs_handle_t handle;
    esp_err_t err = nvs_open("configuration", NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_WARNING, "NVS open failed or empty (%d)", err);
        return;
    }

    for (auto& [name, data] : mDataMap)
    {
        esp_err_t get_err = ESP_FAIL;
        const char* key = name.c_str();

        if (data.GetType() == Data::eType::ebool)
        {
            uint8_t val;
            get_err = nvs_get_u8(handle, key, &val);
            if (get_err == ESP_OK)
            {
                data.SetValue<bool>(val != 0);
            }
        }
        else if (data.GetType() == Data::eType::euint)
        {
            uint32_t val;
            get_err = nvs_get_u32(handle, key, &val);
            if (get_err == ESP_OK)
            {
                data.SetValue<uint32_t>(val);
            }
        } 
        else if (data.GetType() == Data::eType::efloat)
        {
            float val;
            size_t size = sizeof(float);
            get_err = nvs_get_blob(handle, key, &val, &size);
            if (get_err == ESP_OK)
            {
                data.SetValue<float>(val);
            }
        }

        if (get_err == ESP_OK)
        {
            DebugLogger::GetInstance().print(DEBUG_EEPROM, DEBUG_INFO, "Loaded '%s' from Flash", key);
        }
    }
    nvs_close(handle);
}
