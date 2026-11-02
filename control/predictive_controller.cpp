#include "predictive_controller.h"
#include "logging/logger.h"
#include "esp_timer.h"
#include <cmath>
#include <algorithm>

static const char* TAG = "PredictiveController";

PredictiveController& PredictiveController::getInstance() {
    static PredictiveController instance;
    return instance;
}

bool PredictiveController::initialize(uint8_t num_heizkreise) {
    Logger::log(TAG, "Initialisiere Predictive Controller für %d Heizkreise",
                num_heizkreise);
    
    _num_heizkreise = num_heizkreise;
    _control_states.resize(num_heizkreise);
    _desired_valve_states.resize(num_heizkreise, false);
    
    for (uint8_t i = 0; i < num_heizkreise; i++) {
        _control_states[i].predicted_temp = 20.0f;
        _control_states[i].desired_power = 0;
        _control_states[i].valve_desired_state = false;
        _control_states[i].last_update_time = esp_timer_get_time() / 1000;
    }
    
    _last_cycle_time = esp_timer_get_time() / 1000;
    _initialized = true;
    
    Logger::log(TAG, "✓ Controller initialisiert");
    return true;
}

bool* PredictiveController::update(const float* current_temps, const float* setpoints,
                                   const float* return_temps, float flow_temp,
                                   const bool* current_valve_states) {
    if (!_initialized || !current_temps || !setpoints || !return_temps || !current_valve_states) {
        return _desired_valve_states.data();
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    uint32_t cycle_time_ms = now - _last_cycle_time;
    _last_cycle_time = now;
    
    ThermalModel& thermal_model = ThermalModel::getInstance();
    TimeConstantEstimator& time_estimator = TimeConstantEstimator::getInstance();
    
    // Update each circuit
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        ControlState& state = _control_states[i];
        
        float current_temp = current_temps[i];
        float setpoint = setpoints[i];
        float return_temp = return_temps[i];
        bool valve_open = current_valve_states[i];
        
        // Get estimated time constant for this circuit
        uint32_t tau_seconds = time_estimator.getTimeConstant(i);
        
        // Predict room temperature after dead-time period
        float predicted_temp = thermal_model.predictRoomTemperature(
            current_temp, flow_temp, valve_open, VALVE_DEAD_TIME_MS);
        
        // Apply dead-time compensation
        predicted_temp = _compensateDeadTime(i, predicted_temp);
        
        state.predicted_temp = predicted_temp;
        
        // Calculate desired heating power based on error
        float temp_error = setpoint - current_temp;
        state.desired_power = _calculateDesiredPower(current_temp, setpoint, predicted_temp);
        
        // Determine valve state: open if power > 0 and temp below setpoint
        bool power_needed = (state.desired_power > 0) && (temp_error > -HYSTERESIS_ROOM_TEMP);
        state.valve_desired_state = power_needed;
        
        // Update desired valve states array
        _desired_valve_states[i] = state.valve_desired_state;
        
        if (i == 0) {  // Log first circuit as sample
            Logger::log(TAG, 
                       "HK%d: T=%.1f→%.1f°C, pred=%.1f°C, power=%d%%, valve=%s",
                       i, current_temp, setpoint, predicted_temp, 
                       state.desired_power, state.valve_desired_state ? "ON" : "OFF");
        }
        
        state.last_update_time = now;
    }
    
    return _desired_valve_states.data();
}

float PredictiveController::_compensateDeadTime(uint8_t heizkreis_id, 
                                                 float predicted_temp) {
    // Dead-time compensation strategy:
    // If valve is open, the prediction already accounts for heating.
    // If valve just opened, pre-emptively increase prediction to account
    // for the dead-time lag in thermal response.
    
    ControlState& state = _control_states[heizkreis_id];
    
    // Simple compensation: if desired state differs from actual,
    // the system is in transition. Adjust prediction slightly.
    // This is a conservative approach to avoid overshoot.
    
    return predicted_temp;  // Direct prediction; dead-time already in model
}

uint8_t PredictiveController::_calculateDesiredPower(float current_temp, 
                                                      float setpoint,
                                                      float predicted_temp) {
    // Hysteretic two-point control with prediction look-ahead
    
    float temp_error = setpoint - current_temp;
    float future_error = setpoint - predicted_temp;
    
    // If future error will be large, heat fully
    if (future_error > HYSTERESIS_ROOM_TEMP) {
        return 100;
    }
    
    // If future error will be satisfied or exceeded, don't heat
    if (future_error < -HYSTERESIS_ROOM_TEMP) {
        return 0;
    }
    
    // Transition band: proportional to error magnitude
    if (temp_error > 0) {
        uint8_t power = (uint8_t)(std::min(100.0f, 
                       (temp_error / HYSTERESIS_ROOM_TEMP) * 100.0f));
        return power;
    }
    
    return 0;
}

uint8_t PredictiveController::getDesiredHeatingPower(uint8_t heizkreis_id) const {
    if (heizkreis_id >= _control_states.size()) {
        return 0;
    }
    return _control_states[heizkreis_id].desired_power;
}

float PredictiveController::getPredictedTemperature(uint8_t heizkreis_id) const {
    if (heizkreis_id >= _control_states.size()) {
        return 20.0f;
    }
    return _control_states[heizkreis_id].predicted_temp;
}

void PredictiveController::setThermalParameters(float room_thermal_resistance,
                                                float room_thermal_capacity,
                                                float floor_efficiency) {
    ThermalModel::getInstance().setThermalParameters(
        room_thermal_resistance, room_thermal_capacity, floor_efficiency);
    
    Logger::log(TAG, "✓ Thermale Parameter aktualisiert im PredictiveController");
}
