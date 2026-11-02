#pragma once

#include <stdint.h>
#include "config.h"
#include "dynamics/thermal_model.h"
#include "dynamics/time_constant_estimator.h"

// Inner-loop predictive controller with dead-time compensation
// Uses thermal model to predict room temperature and adjust valve position proactively
class PredictiveController {
public:
    static PredictiveController& getInstance();
    
    // Initialize with number of circuits
    bool initialize(uint8_t num_heizkreise);
    
    // Update controller state and return desired valve positions
    // Parameters:
    //   current_temps: current room temperatures [°C]
    //   setpoints: target room temperatures [°C]
    //   return_temps: return water temperatures [°C]
    //   flow_temp: flow (supply) water temperature [°C]
    //   current_valve_states: current valve open/closed states
    // Returns: array of desired valve states (true=open, false=closed)
    bool* update(const float* current_temps, const float* setpoints,
                 const float* return_temps, float flow_temp,
                 const bool* current_valve_states);
    
    // Get desired heating power for a circuit (0-100%)
    uint8_t getDesiredHeatingPower(uint8_t heizkreis_id) const;
    
    // Get predicted room temperature for a circuit (look-ahead)
    float getPredictedTemperature(uint8_t heizkreis_id) const;
    
    // Recalibrate thermal parameters
    void setThermalParameters(float room_thermal_resistance,
                              float room_thermal_capacity,
                              float floor_efficiency);

private:
    PredictiveController() = default;
    ~PredictiveController() = default;
    PredictiveController(const PredictiveController&) = delete;
    void operator=(const PredictiveController&) = delete;
    
    struct ControlState {
        float predicted_temp = 20.0f;
        uint8_t desired_power = 0;
        bool valve_desired_state = false;
        uint32_t last_update_time = 0;
    };
    
    std::vector<ControlState> _control_states;
    std::vector<bool> _desired_valve_states;
    uint8_t _num_heizkreise = 0;
    bool _initialized = false;
    uint32_t _last_cycle_time = 0;
    
    // Internal: dead-time compensation using valve state history
    float _compensateDeadTime(uint8_t heizkreis_id, float predicted_temp);
    
    // Internal: calculate desired power based on temperature error
    uint8_t _calculateDesiredPower(float current_temp, float setpoint, 
                                    float predicted_temp);
};
