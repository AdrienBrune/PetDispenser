
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_zigbee_core.h"
#include "algorithm"
#include "string.h"
#include "nvs_flash.h"
#include "esp_systick_etm.h"
#include "debug.hpp"
#include "zigbee.hpp"
#include "memory/pers_mem.hpp"
#include "freertos/queue.h"
#include "led.hpp"
#include "Dispenser.hpp"

#define ZB_BTN_PIN 26
#define DISPENSE_BTN_PIN 27

static QueueHandle_t gpio_evt_queue = NULL;
typedef struct {
    uint32_t pin;
    uint32_t level;
    TickType_t timestamp;
} button_event_t;

void _task_zigbee(void *pvParameters)
{
    if(initZigbee() != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_TASK, DEBUG_ERROR, "ZIGBEE init failure");
        return;
    }

    if(initDevice() != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_TASK, DEBUG_ERROR, "ZIGBEE device registration failure");
        return;
    }

    esp_zb_set_rx_on_when_idle(false);

    if(esp_zb_start(false) != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_TASK, DEBUG_ERROR, "ZIGBEE hasn't started");
        return;
    }

    DebugLogger::GetInstance().print(DEBUG_TASK, DEBUG_INFO, "ZIGBEE main loop started");
    esp_zb_stack_main_loop();
}


void _task_dispenser(void *pvParameters)
{
    Dispenser& dispenser = Dispenser::GetInstance();

    dispenser.Init();
    dispenser.Routine();
}


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    button_event_t ev;
    ev.pin = gpio_num;
    ev.level = gpio_get_level((gpio_num_t)gpio_num);
    ev.timestamp = xTaskGetTickCountFromISR();
    
    // send the event via queue
    xQueueSendFromISR(gpio_evt_queue, &ev, NULL);
}

void _task_button(void *pvParameters)
{
    button_event_t ev;
    TickType_t zbBtnPressStartTicks = 0;

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << ZB_BTN_PIN) | (1ULL << DISPENSE_BTN_PIN);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(button_event_t));

    // start ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)ZB_BTN_PIN, gpio_isr_handler, (void*) ZB_BTN_PIN);

    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &ev, portMAX_DELAY))
        {
            DebugLogger::GetInstance().print(DEBUG_BUTTONS, DEBUG_INFO, "Button event detected: %s", ev.pin == ZB_BTN_PIN ? "zigbee buttin" : "dispense button");
            
            ev.level == 0 ? 
                DebugLogger::GetInstance().print(DEBUG_BUTTONS, DEBUG_INFO, "Button pressed") :
                DebugLogger::GetInstance().print(DEBUG_BUTTONS, DEBUG_INFO, "Button released");

            if (ev.pin == ZB_BTN_PIN)
            {
                if (ev.level == 0) // rising edge
                {
                    zbBtnPressStartTicks = ev.timestamp;
                    DebugLogger::GetInstance().print(DEBUG_BUTTONS, DEBUG_INFO, "Button pressed");
                }
                else // falling edge
                {
                    TickType_t pressDuration = ev.timestamp - zbBtnPressStartTicks;
                    uint32_t duration_ms = pdTICKS_TO_MS(pressDuration);

                    DebugLogger::GetInstance().print(DEBUG_BUTTONS, DEBUG_INFO, "Button released after %lu ms", duration_ms);

                    if (duration_ms > 5000)
                    {
                        DebugLogger::GetInstance().print(DEBUG_BUTTONS, DEBUG_INFO, "Action: Long press detected (Factory Reset)");
                        if (esp_zb_lock_acquire(portMAX_DELAY))
                        {
                            Led& led = Led::GetInstance();
                            led.Set(ZB_LED_PIN, 0);
                            vTaskDelay(pdMS_TO_TICKS(500));
                            led.Set(ZB_LED_PIN, 1);
                            vTaskDelay(pdMS_TO_TICKS(500));
                            led.Set(ZB_LED_PIN, 0);
                            vTaskDelay(pdMS_TO_TICKS(500));
                            led.Set(ZB_LED_PIN, 1);
                            vTaskDelay(pdMS_TO_TICKS(500));
                            led.Set(ZB_LED_PIN, 0);
                            vTaskDelay(pdMS_TO_TICKS(500));
                            led.Set(ZB_LED_PIN, 1);
                            vTaskDelay(pdMS_TO_TICKS(500));

                            connected.store(false);
                            esp_zb_factory_reset();
                            esp_zb_lock_release();
                        }
                    }
                    else if (duration_ms > 50)
                    {
                        DebugLogger::GetInstance().print(DEBUG_BUTTONS, DEBUG_INFO, "Action: Short press detected (Force Reporting)");
                        Dispenser::GetInstance().ZigbeeReport();
                    }
                }
            }
            else // DISPENSE_BTN_PIN
            {
                if (ev.level == 0) // rising edge
                {
                    Dispenser::GetInstance().DispenseFoodContinue(true);
                }
                else // falling edge
                {
                    Dispenser::GetInstance().DispenseFoodContinue(false);
                }
            }
            
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(4000));

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        DebugLogger::GetInstance().print(DEBUG_GENERIC, DEBUG_ERROR, "NVS flash init");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_GENERIC, DEBUG_ERROR, "NVS init failure, program stopped");
        return;
    }

    esp_zb_platform_config_t config = {
        .radio_config = { .radio_mode = ZB_RADIO_MODE_NATIVE },
        .host_config = { .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE },
    };
    if(esp_zb_platform_config(&config) != ESP_OK)
    {
        DebugLogger::GetInstance().print(DEBUG_GENERIC, DEBUG_ERROR, "ZIGBEE init failure, program stopped");
        return;
    }

    Memory::GetMemory().Load();

    Led::GetInstance().Init((gpio_num_t)ZB_LED_PIN);

    DebugLogger::GetInstance().print(DEBUG_GENERIC, DEBUG_INFO, "init completed");

    xTaskCreatePinnedToCore(_task_dispenser, "dispenser", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(_task_button, "button", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(_task_zigbee, "zigbee", 8124, NULL, 3, NULL, 0);
}
