#pragma once

#include <stdint.h>
#include <vector>
#include "config.h"
#include "heizkreis.h"

// Central two-point controller managing all heating circuits and pump
// Coordinates valve switching, hysteresis logic, and pump control
class TwoPointController {
public:
    static TwoPointController& getInstance();
    
    // Initialize with number of circuits
    bool initialize(uint8_t num_heizkreise);
    
    // Main control loop update (call periodically)
    // Parameters:
    //   room_temps: current room temperatures [°C]
    //   setpoints: target room temperatures [°C]
    //   return_temps: return water temperatures [°C]
    //   flow_temp: flow (supply) water temperature [°C]
    void update(const float* room_temps, const float* setpoints,
               const float* return_temps, float flow_temp);
    
    // Set target temperature for a circuit
    void setSetpoint(uint8_t heizkreis_id, float setpoint_celsius);
    
    // Get circuit state
    HeizkreisState getCircuitState(uint8_t heizkreis_id) const;
    bool isCircuitValveOpen(uint8_t heizkreis_id) const;
    float getCircuitReturnTemperature(uint8_t heizkreis_id) const;
    
    // Pump control
    bool isPumpRunning() const;
    
    // Emergency shutdown
    void emergencyStop();
    
    // Get active circuit count (circuits with open valves)
    uint8_t getActiveCircuitCount() const;

private:
    TwoPointController() = default;
    ~TwoPointController() = default;
    TwoPointController(const TwoPointController&) = delete;
    void operator=(const TwoPointController&) = delete;
    
    std::vector<Heizkreis> _heizkreise;
    uint8_t _num_heizkreise = 0;
    bool _pump_running = false;
    uint32_t _pump_shutdown_requested_time = 0;
    bool _initialized = false;
    uint32_t _last_update_time = 0;
    
    // Internal: check and update pump state
    void _updatePumpState();
    
    // Internal: determine if pump should run
    bool _shouldPumpRun() const;
};
