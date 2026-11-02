#include "thermal_model.h"
#include "logging/logger.h"
#include <cmath>

static const char* TAG = "ThermalModel";

ThermalModel& ThermalModel::getInstance() {
    static ThermalModel instance;
    return instance;
}

bool ThermalModel::initialize() {
    Logger::log(TAG, "Initialisiere Thermales Modell");
    Logger::log(TAG, "  R_room = %.3f K/W", _R_room);
    Logger::log(TAG, "  C_room = %.0f J/K", _C_room);
    Logger::log(TAG, "  η = %.1f%%", _efficiency * 100);
    Logger::log(TAG, "  P_on = %.0f W, P_off = %.0f W", 
               _heater_power_on, _heater_power_off);
    
    _initialized = true;
    return true;
}

float ThermalModel::predictRoomTemperature(float current_temp, float flow_temp,
                                           bool valve_open, uint32_t delta_time_ms) {
    if (!_initialized) {
        return current_temp;
    }
    
    // Determine active heating power based on valve state
    float active_power = valve_open ? _heater_power_on : _heater_power_off;
    
    // Effective thermal power transfer to room:
    // P_eff = (flow_temp - current_temp) / R_room
    // This models heat transfer through the floor circuit
    float temp_differential = flow_temp - current_temp;
    float power_transfer = temp_differential / _R_room;
    
    // Apply efficiency factor to actual heating power
    float total_power = (active_power * _efficiency) + power_transfer;
    
    // Differential equation: C_room * dT/dt = P_total
    // Solution: dT = (P_total / C_room) * Δt
    float delta_time_seconds = delta_time_ms / 1000.0f;
    float temp_change = (total_power / _C_room) * delta_time_seconds;
    
    float predicted_temp = current_temp + temp_change;
    
    return predicted_temp;
}

void ThermalModel::setThermalParameters(float room_thermal_resistance,
                                        float room_thermal_capacity,
                                        float floor_efficiency) {
    _R_room = room_thermal_resistance;
    _C_room = room_thermal_capacity;
    _efficiency = floor_efficiency;
    
    Logger::log(TAG, "✓ Thermale Parameter aktualisiert: R=%.3f, C=%.0f, η=%.1f%%",
               _R_room, _C_room, _efficiency * 100);
}
