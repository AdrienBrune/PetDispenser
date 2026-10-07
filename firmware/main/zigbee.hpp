#pragma once

#include "ha/esp_zigbee_ha_standard.h"
#include "zcl_utility.h"
#include "zcl/esp_zigbee_zcl_command.h"
#include "zcl/esp_zigbee_zcl_ias_zone.h"
#include "zcl/esp_zigbee_zcl_power_config.h"
#include "esp_zigbee_core.h"
#include "atomic"

#define ZB_EP_USER                       1
#define ZB_EP_CONFIG                     2

#define CUSTOM_CLUSTER_ID           0xFF00 

#define ATTR_BUTTON_DISPENSE_ID     0x0000
#define ATTR_MOTOR_SPEED_ID         0x0001
#define ATTR_TURN_PER_PORTION_ID    0x0002 

extern std::atomic<bool> connected;

esp_err_t initZigbee();
esp_err_t initDevice();
void updateTankFilling(uint8_t endpoint, float value);
void updateMotorSpeed(uint8_t endpoint, uint32_t value);
void updateTurnPerPortion(uint8_t endpoint, float value);
void updatePortionWeight(uint8_t endpoint, float value);
void updateSleepMode(uint8_t endpoint, bool value);