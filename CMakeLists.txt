# Main project CMakeLists.txt for ESP32-S3 floor heating controller

cmake_minimum_required(VERSION 3.16)
project(floorheating_esp32s3)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============= COMPONENT SOURCES =============

set(SOURCES
    # Hardware layer
    hardware/ds18b20_sensors.cpp
    hardware/relay_control.cpp
    
    # Wireless receivers
    wireless/mobile_alerts_receiver.cpp
    wireless/wifi_manager.cpp
    
    # Control logic
    control/heizkreis.cpp
    control/two_point_controller.cpp
    control/predictive_controller.cpp
    control/heizgradkurve.cpp
    
    # System dynamics
    dynamics/time_constant_estimator.cpp
    dynamics/thermal_model.cpp
    
    # Web interface
    web/webserver.cpp
    web/api_handler.cpp
    
    # System management
    system/data_manager.cpp
    system/nvs_storage.cpp
    
    # Logging
    logging/logger.cpp
    
    # Main application
    main.cpp
)

set(HEADERS
    config.h
    hardware/ds18b20_sensors.h
    hardware/relay_control.h
    hardware/pin_definitions.h
    wireless/mobile_alerts_receiver.h
    wireless/wifi_manager.h
    control/heizkreis.h
    control/two_point_controller.h
    control/predictive_controller.h
    control/heizgradkurve.h
    dynamics/time_constant_estimator.h
    dynamics/thermal_model.h
    web/webserver.h
    web/api_handler.h
    system/data_manager.h
    system/nvs_storage.h
    logging/logger.h
)

# ============= CREATE COMPONENT =============

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS .
    REQUIRES 
        esp_http_server
        cjson
        driver
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
)

# ============= EMBED WEB ASSETS =============

# Embed index.html into the binary
target_add_binary_data(
    ${COMPONENT_LIB}
    "web/assets/index.html"
    TEXT
)

# ============= COMPILER FLAGS =============

# Enable all warnings
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic)

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${COMPONENT_LIB} PRIVATE -O3)
else()
    target_compile_options(${COMPONENT_LIB} PRIVATE -O0 -g)
endif()
