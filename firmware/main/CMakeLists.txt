idf_component_register(
    SRCS "Dispenser.cpp" "zigbee.cpp" "VL53L1X/VL53L1X_api.c" "VL53L1X/VL53L1X_calibration.c" "VL53L1X/vl53l1_platform.c" "main.cpp" "Dispenser.cpp" "Nema17.cpp" "infraredsensor.cpp" "memory/pers_mem.cpp"
    INCLUDE_DIRS "." "VL53L1X"
    PRIV_REQUIRES nvs_flash esp_driver_uart esp_driver_i2c esp_driver_gpio esp_timer ieee802154
)
