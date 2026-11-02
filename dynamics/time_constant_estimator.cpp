#include "time_constant_estimator.h"
#include "logging/logger.h"
#include "esp_timer.h"
#include <cmath>
#include <algorithm>

static const char* TAG = "TimeConstEstimator";

TimeConstantEstimator& TimeConstantEstimator::getInstance() {
    static TimeConstantEstimator instance;
    return instance;
}

bool TimeConstantEstimator::initialize(uint8_t num_heizkreise) {
    Logger::log(TAG, "Initialisiere TimeConstant-Schätzer für %d Heizkreise", 
                num_heizkreise);
    
    _estimations.resize(num_heizkreise);
    
    for (uint8_t i = 0; i < num_heizkreise; i++) {
        _estimations[i].estimated_time_constant = 300;  // Default 5 Min
        _estimations[i].estimation_valid = false;
        _estimations[i].last_valve_change_time = 0;
    }
    
    _last_update_time = esp_timer_get_time() / 1000;
    _initialized = true;
    
    Logger::log(TAG, "✓ Schätzer initialisiert");
    return true;
}

void TimeConstantEstimator::update(const float* return_temps, const bool* valve_states) {
    if (!_initialized || !return_temps || !valve_states) {
        return;
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    
    // Führe Update nur alle TIME_CONSTANT_ESTIMATION_INTERVAL_MS durch
    if ((now - _last_update_time) < TIME_CONSTANT_ESTIMATION_INTERVAL_MS) {
        return;
    }
    _last_update_time = now;
    
    // Aktualisiere für jeden Heizkreis
    for (size_t i = 0; i < _estimations.size(); i++) {
        EstimationState& est = _estimations[i];
        
        // Erkenne Ventilstellungsänderung
        if (valve_states[i] != est.valve_was_open) {
            est.last_valve_change_time = now;
            est.valve_was_open = valve_states[i];
            est.return_temp_history.clear();
            est.samples_collected = 0;
            
            const char* action = valve_states[i] ? "OFFEN" : "GESCHLOSSEN";
            Logger::log(TAG, "HK%zu: Ventil %s → Neue Schätzung startet", i, action);
        }
        
        // Sammle Daten nach Ventilstörung für mindestens 2x Zeitkonstante
        uint32_t elapsed = now - est.last_valve_change_time;
        if (elapsed > 0 && elapsed < (TIME_CONSTANT_MAX_SECONDS * 1000 * 2)) {
            est.return_temp_history.push_back(return_temps[i]);
            est.samples_collected++;
            
            // Schätzung bei ausreichend Daten
            if (est.samples_collected > 10) {
                _estimateTimeConstant(i, return_temps[i]);
            }
        }
    }
}

void TimeConstantEstimator::_estimateTimeConstant(uint8_t heizkreis_id, 
                                                   float current_return_temp) {
    EstimationState& est = _estimations[heizkreis_id];
    
    if (est.return_temp_history.size() < 5) {
        return;
    }
    
    // Berechne mittlere Änderungsrate (dT/dt)
    float initial_temp = est.return_temp_history.front();
    float current_delta = current_return_temp - initial_temp;
    
    // Wenn minimal Temperaturänderung: Abbrechen
    if (std::abs(current_delta) < MIN_FLOW_CHANGE_FOR_ESTIMATION) {
        return;
    }
    
    // Exponentielles Modell: T(t) = T_∞ - ΔT * e^(-t/τ)
    // Vereinfachte Schätzung basierend auf Anstiegsgeschwindigkeit
    
    // Finde 63,2% Punkt (eine Zeitkonstante)
    float target_delta = current_delta * 0.632f;
    
    uint32_t time_to_63_percent = 0;
    for (size_t i = 1; i < est.return_temp_history.size(); i++) {
        float delta_i = est.return_temp_history[i] - initial_temp;
        if (std::abs(delta_i) >= std::abs(target_delta)) {
            time_to_63_percent = i * TIME_CONSTANT_ESTIMATION_INTERVAL_MS;
            break;
        }
    }
    
    if (time_to_63_percent > 0) {
        uint32_t tau_seconds = time_to_63_percent / 1000;
        
        // Begrenze auf zulässigen Bereich
        tau_seconds = std::max((uint32_t)TIME_CONSTANT_MIN_SECONDS,
                              std::min((uint32_t)TIME_CONSTANT_MAX_SECONDS, tau_seconds));
        
        est.estimated_time_constant = tau_seconds;
        est.estimation_valid = true;
        
        Logger::log(TAG, "HK%d: ✓ τ = %u Sekunden geschätzt (ΔT=%.1f°C)",
                   heizkreis_id, tau_seconds, current_delta);
    }
}

uint32_t TimeConstantEstimator::getTimeConstant(uint8_t heizkreis_id) {
    if (heizkreis_id >= _estimations.size()) {
        return 300;  // Default
    }
    return _estimations[heizkreis_id].estimated_time_constant;
}

bool TimeConstantEstimator::isEstimationReady(uint8_t heizkreis_id) {
    if (heizkreis_id >= _estimations.size()) {
        return false;
    }
    return _estimations[heizkreis_id].estimation_valid;
}

void TimeConstantEstimator::resetEstimation(uint8_t heizkreis_id) {
    if (heizkreis_id >= _estimations.size()) {
        return;
    }
    
    _estimations[heizkreis_id].return_temp_history.clear();
    _estimations[heizkreis_id].estimated_time_constant = 300;
    _estimations[heizkreis_id].estimation_valid = false;
    _estimations[heizkreis_id].samples_collected = 0;
    
    Logger::log(TAG, "HK%d: Schätzung zurückgesetzt", heizkreis_id);
}
