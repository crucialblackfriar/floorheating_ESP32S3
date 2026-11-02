#pragma once

#include <stdint.h>
#include <vector>
#include "config.h"

// Non-Volatile Storage manager for persistent configuration
// Stores setpoints, time constants, and calibration data
class NVSStorage {
public:
    static NVSStorage& getInstance();
    
    // Initialize NVS partition
    bool initialize();
    
    // Setpoint management
    bool saveSetpoints(const float* setpoints, uint8_t num_circuits);
    bool loadSetpoints(float* setpoints, uint8_t num_circuits);
    
    // Time constant management
    bool saveTimeConstants(const uint32_t* time_constants, uint8_t num_circuits);
    bool loadTimeConstants(uint32_t* time_constants, uint8_t num_circuits);
    
    // Valve calibration data
    bool saveValveCalibration(const float* calibration_data, uint8_t num_circuits);
    bool loadValveCalibration(float* calibration_data, uint8_t num_circuits);
    
    // Thermal model parameters
    bool saveThermalParameters(float room_resistance, float room_capacity, 
                               float efficiency);
    bool loadThermalParameters(float& room_resistance, float& room_capacity,
                               float& efficiency);
    
    // Clear all stored data
    bool clearAll();

private:
    NVSStorage() = default;
    ~NVSStorage() = default;
    NVSStorage(const NVSStorage&) = delete;
    void operator=(const NVSStorage&) = delete;
    
    bool _initialized = false;
    
    // Internal: validate stored data
    bool _validateChecksum(const uint8_t* data, uint32_t length, 
                           uint32_t checksum) const;
    uint32_t _calculateChecksum(const uint8_t* data, uint32_t length) const;
};
