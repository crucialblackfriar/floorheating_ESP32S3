#pragma once

#include <stdint.h>
#include <vector>
#include "config.h"

// Central data manager for sensor readings and system state
// Collects data from all sources (DS18B20, Mobile Alerts, controllers)
class DataManager {
public:
    static DataManager& getInstance();
    
    // Initialize data manager
    bool initialize(uint8_t num_heizkreise);
    
    // Update sensor readings
    void updateRoomTemperatures(const float* temps);
    void updateReturnTemperatures(const float* temps);
    void updateFlowTemperature(float temp);
    void updateOutdoorTemperature(float temp);
    void updateValveStates(const bool* states);
    void updateSetpoints(const float* setpoints);
    
    // Read current sensor values
    float getRoomTemperature(uint8_t heizkreis_id) const;
    float getReturnTemperature(uint8_t heizkreis_id) const;
    float getFlowTemperature() const;
    float getOutdoorTemperature() const;
    bool getValveState(uint8_t heizkreis_id) const;
    float getSetpoint(uint8_t heizkreis_id) const;
    
    // Timestamp of last valid reading
    uint32_t getLastUpdateTime() const;
    
    // Detect sensor failures
    bool isRoomTemperatureSensorValid(uint8_t heizkreis_id) const;
    bool isReturnTemperatureSensorValid(uint8_t heizkreis_id) const;
    bool isFlowTemperatureSensorValid() const;
    bool isOutdoorTemperatureSensorValid() const;
    
    // Get sensor data as JSON string (for web interface)
    const char* getSensorDataJson();

private:
    DataManager() = default;
    ~DataManager() = default;
    DataManager(const DataManager&) = delete;
    void operator=(const DataManager&) = delete;
    
    struct SensorData {
        float room_temp = 20.0f;
        float return_temp = 40.0f;
        float flow_temp = 50.0f;
        float outdoor_temp = 10.0f;
        bool valve_state = false;
        float setpoint = 21.0f;
        uint32_t last_update_time = 0;
        bool is_valid = false;
    };
    
    std::vector<SensorData> _circuit_data;
    float _flow_temperature = 50.0f;
    float _outdoor_temperature = 10.0f;
    uint32_t _flow_temp_last_update = 0;
    uint32_t _outdoor_temp_last_update = 0;
    uint8_t _num_heizkreise =
