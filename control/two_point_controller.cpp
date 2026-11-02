#include "two_point_controller.h"
#include "logging/logger.h"
#include "esp_timer.h"

static const char* TAG = "TwoPointController";

TwoPointController& TwoPointController::getInstance() {
    static TwoPointController instance;
    return instance;
}

bool TwoPointController::initialize(std::vector<Heizkreis>* heizkreise) {
    if (!heizkreise || heizkreise->size() != NUM_HEIZKREISE) {
        Logger::log(TAG, "✗ Ungültige Heizkreis-Liste");
        return false;
    }
    
    _heizkreise = heizkreise;
    _pump_on = false;
    _pump_switch_time = 0;
    
    Logger::log(TAG, "✓ TwoPointController mit %zu Heizkreisen initialisiert", 
                heizkreise->size());
    return true;
}

void TwoPointController::update(float vorlauf_temp, float* return_temps,
                                float outdoor_temp, float* room_temps) {
    if (!_heizkreise) return;
    
    // Aktualisiere jeden Heizkreis
    for (size_t i = 0; i < _heizkreise->size(); i++) {
        float return_temp = return_temps ? return_temps[i] : 40.0f;
        float room_temp = room_temps ? room_temps[i] : 20.0f;
        
        (*_heizkreise)[i].update(room_temp, vorlauf_temp, return_temp);
    }
    
    // Prüfe Pump-Status: Mindestens ein Ventil offen?
    bool any_valve_open = false;
    for (const auto& hk : *_heizkreise) {
        if (hk.isValveOpen()) {
            any_valve_open = true;
            break;
        }
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    
    // Pumpen-Schaltlogik mit Verzögerung
    if (any_valve_open && !_pump_on) {
        _pump_on = true;
        _pump_switch_time = now;
        Logger::log(TAG, "Pumpe: EIN");
    } else if (!any_valve_open && _pump_on) {
        // Verzögere Pump-Abschaltung (Auskühlschutz)
        if ((now - _pump_switch_time) > PUMP_SHUTDOWN_DELAY_MS) {
            _pump_on = false;
            _pump_switch_time = 0;
            Logger::log(TAG, "Pumpe: AUS (Nach Verzögerung)");
        }
    }
}

bool TwoPointController::shouldPumpBeOn() {
    return _pump_on;
}

void TwoPointController::printStatus() {
    if (!_heizkreise) return;
    
    Logger::log(TAG, "--- REGELSTATUS ---");
    Logger::log(TAG, "Pumpe: %s", _pump_on ? "EIN" : "AUS");
    
    for (size_t i = 0; i < _heizkreise->size(); i++) {
        const auto& hk = (*_heizkreise)[i];
        const char* state_str = "";
        
        switch (hk.getState()) {
            case HeizkreisState::IDLE: state_str = "IDLE"; break;
            case HeizkreisState::OPENING: state_str = "OPENING"; break;
            case HeizkreisState::OPEN: state_str = "OPEN"; break;
            case HeizkreisState::CLOSING: state_str = "CLOSING"; break;
            case HeizkreisState::ERROR: state_str = "ERROR"; break;
        }
        
        Logger::log(TAG, "HK%zu: Soll=%.1f°C, Ist=%.1f°C, Ventil=%s (%.0f%%), τ=%us",
                   i, hk.getSetpoint(), hk.getRoomTemperature(),
                   state_str, (float)hk.getHeatingPower(), hk.getTimeConstant());
    }
}
