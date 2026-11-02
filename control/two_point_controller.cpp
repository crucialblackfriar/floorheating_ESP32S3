#include "two_point_controller.h"
#include "logging/logger.h"
#include "esp_timer.h"
#include "hardware/relay_control.h"

static const char* TAG = "TwoPointController";

TwoPointController& TwoPointController::getInstance() {
    static TwoPointController instance;
    return instance;
}

bool TwoPointController::initialize(uint8_t num_heizkreise) {
    Logger::log(TAG, "Initialisiere TwoPointController für %d Heizkreise",
                num_heizkreise);
    
    _num_heizkreise = num_heizkreise;
    _heizkreise.clear();
    
    for (uint8_t i = 0; i < num_heizkreise; i++) {
        _heizkreise.emplace_back(i);
        _heizkreise[i].initialize();
    }
    
    _pump_running = false;
    _pump_shutdown_requested_time = 0;
    _last_update_time = esp_timer_get_time() / 1000;
    _initialized = true;
    
    Logger::log(TAG, "✓ Controller mit %d Heizkreisen initialisiert",
                num_heizkreise);
    return true;
}

void TwoPointController::update(const float* room_temps, const float* setpoints,
                               const float* return_temps, float flow_temp) {
    if (!_initialized || !room_temps || !setpoints || !return_temps) {
        return;
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    _last_update_time = now;
    
    // Update each circuit
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        _heizkreise[i].update(room_temps[i], flow_temp, return_temps[i]);
        _heizkreise[i].setSetpoint(setpoints[i]);
    }
    
    // Check and update pump state
    _updatePumpState();
    
    Logger::log(TAG, "Zyklus: %d Heizkreise aktiv, Pumpe: %s",
               getActiveCircuitCount(), _pump_running ? "AN" : "AUS");
}

void TwoPointController::_updatePumpState() {
    bool pump_needed = _shouldPumpRun();
    
    if (pump_needed && !_pump_running) {
        // Start pump
        RelayControl::getInstance().setPumpRelay(true);
        _pump_running = true;
        _pump_shutdown_requested_time = 0;
        Logger::log(TAG, "✓ Pumpe eingeschaltet");
    } 
    else if (!pump_needed && _pump_running) {
        // Request pump shutdown with delay
        if (_pump_shutdown_requested_time == 0) {
            _pump_shutdown_requested_time = esp_timer_get_time() / 1000;
            Logger::log(TAG, "Pumpen-Abschaltung angefordert (Verzögerung: %u ms)",
                       PUMP_SHUTDOWN_DELAY_MS);
        } else {
            uint32_t elapsed = (esp_timer_get_time() / 1000) - _pump_shutdown_requested_time;
            if (elapsed >= PUMP_SHUTDOWN_DELAY_MS) {
                RelayControl::getInstance().setPumpRelay(false);
                _pump_running = false;
                _pump_shutdown_requested_time = 0;
                Logger::log(TAG, "✓ Pumpe ausgeschaltet");
            }
        }
    } 
    else if (pump_needed && _pump_running) {
        // Pump running and still needed: cancel any shutdown request
        _pump_shutdown_requested_time = 0;
    }
}

bool TwoPointController::_shouldPumpRun() const {
    // Pump should run if ANY circuit has an open valve
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        if (_heizkreise[i].isValveOpen()) {
            return true;
        }
    }
    return false;
}

void TwoPointController::setSetpoint(uint8_t heizkreis_id, float setpoint_celsius) {
    if (heizkreis_id < _num_heizkreise) {
        _heizkreise[heizkreis_id].setSetpoint(setpoint_celsius);
    }
}

HeizkreisState TwoPointController::getCircuitState(uint8_t heizkreis_id) const {
    if (heizkreis_id < _num_heizkreise) {
        return _heizkreise[heizkreis_id].getState();
    }
    return HeizkreisState::ERROR;
}

bool TwoPointController::isCircuitValveOpen(uint8_t heizkreis_id) const {
    if (heizkreis_id < _num_heizkreise) {
        return _heizkreise[heizkreis_id].isValveOpen();
    }
    return false;
}

float TwoPointController::getCircuitReturnTemperature(uint8_t heizkreis_id) const {
    if (heizkreis_id < _num_heizkreise) {
        return _heizkreise[heizkreis_id].getReturnTemperature();
    }
    return 0.0f;
}

bool TwoPointController::isPumpRunning() const {
    return _pump_running;
}

uint8_t TwoPointController::getActiveCircuitCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        if (_heizkreise[i].isValveOpen()) {
            count++;
        }
    }
    return count;
}

void TwoPointController::emergencyStop() {
    Logger::log(TAG, "⚠ NOTFALL-STOPP aktiviert!");
    
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        _heizkreise[i].emergencyStop();
    }
    
    RelayControl::getInstance().setPumpRelay(false);
    _pump_running = false;
    
    RelayControl::getInstance().emergencyShutdown();
    Logger::log(TAG, "✓ Alle Ventile und Pumpe deaktiviert");
}
