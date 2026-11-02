#include "heizkreis.h"
#include "relay_control.h"
#include "logger.h"
#include "esp_timer.h"

static const char* TAG = "Heizkreis";

Heizkreis::Heizkreis(uint8_t id) : _id(id) {
    Logger::log(TAG, "Erstelle Heizkreis %d", _id);
}

void Heizkreis::initialize() {
    _state = HeizkreisState::IDLE;
    _valve_is_open = false;
    _state_change_time = esp_timer_get_time() / 1000;
    Logger::log(TAG, "Heizkreis %d initialisiert", _id);
}

void Heizkreis::update(float current_room_temp, float flow_temp, float return_temp) {
    // Speichere aktuelle Werte
    _current_room_temp = current_room_temp;
    _current_flow_temp = flow_temp;
    _current_return_temp = return_temp;
    
    // Berechne Temperaturabweichung
    float temp_diff = _setpoint - current_room_temp;
    
    // Zweipunkt-Regelung
    _updateTwoPointControl(temp_diff);
    
    // Prüfe State-Übergänge
    _checkStateTransitions();
}

void Heizkreis::_updateTwoPointControl(float temp_diff) {
    const float HYSTERESIS = HYSTERESIS_ROOM_TEMP;
    uint32_t now = esp_timer_get_time() / 1000;
    
    // Zweipunkt-Regler mit Hysterese
    if (temp_diff > HYSTERESIS) {
        // Soll ist höher → Ventil öffnen
        if (_state == HeizkreisState::IDLE) {
            _state = HeizkreisState::OPENING;
            _state_change_time = now;
            RelayControl::getInstance().setValve(_id, true);
            _heating_power = 100;
            Logger::log(TAG, "HK%d: Ventil öffnet (T_soll=%.1f°C, T_ist=%.1f°C)",
                       _id, _setpoint, _current_room_temp);
        }
    } else if (temp_diff < -HYSTERESIS) {
        // Soll ist niedriger → Ventil schließen
        if (_state != HeizkreisState::IDLE) {
            _state = HeizkreisState::CLOSING;
            _state_change_time = now;
            RelayControl::getInstance().setValve(_id, false);
            _heating_power = 0;
            Logger::log(TAG, "HK%d: Ventil schließt (T_soll=%.1f°C, T_ist=%.1f°C)",
                       _id, _setpoint, _current_room_temp);
        }
    } else {
        // Im Bereich: Modulieren (wenn Zweipunkt zu heftig ist)
        _heating_power = (int)(50 + (temp_diff / HYSTERESIS) * 50);
    }
}

void Heizkreis::_checkStateTransitions() {
    uint32_t now = esp_timer_get_time() / 1000;
    uint32_t elapsed = now - _state_change_time;
    
    switch (_state) {
        case HeizkreisState::OPENING:
            // Warte bis Ventil vollständig geöffnet ist (Totzeit überwunden)
            if (elapsed >= VALVE_DEAD_TIME_MS) {
                _state = HeizkreisState::OPEN;
                _valve_is_open = true;
                Logger::log(TAG, "HK%d: Ventil vollständig offen", _id);
            }
            break;
            
        case HeizkreisState::OPEN:
            // Wenn Temperatur zu hoch wird → Schließen
            if ((_setpoint - _current_room_temp) < -HYSTERESIS_ROOM_TEMP) {
                _state = HeizkreisState::CLOSING;
                _state_change_time = now;
                RelayControl::getInstance().setValve(_id, false);
                _heating_power = 0;
                Logger::log(TAG, "HK%d: Ventil schließt (Übertemperatur)", _id);
            }
            break;
            
        case HeizkreisState::CLOSING:
            // Ventil ist geschlossen
            if (elapsed >= VALVE_RESPONSE_TIME_MS) {
                _state = HeizkreisState::IDLE;
                _valve_is_open = false;
                _heating_power = 0;
                Logger::log(TAG, "HK%d: Ventil vollständig geschlossen", _id);
            }
            break;
            
        case HeizkreisState::ERROR:
            // Fehler-State: Ventil sofort schließen als Sicherheitsmaßnahme
            RelayControl::getInstance().setValve(_id, false);
            break;
            
        default:
            break;
    }
}

void Heizkreis::setSetpoint(float setpoint_celsius) {
    // Begrenze Sollwert auf zulässige Bereich
    if (setpoint_celsius < SETPOINT_MIN_CELSIUS) {
        setpoint_celsius = SETPOINT_MIN_CELSIUS;
    } else if (setpoint_celsius > SETPOINT_MAX_CELSIUS) {
        setpoint_celsius = SETPOINT_MAX_CELSIUS;
    }
    
    float old_setpoint = _setpoint;
    _setpoint = setpoint_celsius;
    
    if (old_setpoint != _setpoint) {
        Logger::log(TAG, "HK%d: Solltemperatur %.1f°C → %.1f°C", 
                   _id, old_setpoint, _setpoint);
    }
}

float Heizkreis::getSetpoint() const {
    return _setpoint;
}

HeizkreisState Heizkreis::getState() const {
    return _state;
}

float Heizkreis::getRoomTemperature() const {
    return _current_room_temp;
}

float Heizkreis::getReturnTemperature() const {
    return _current_return_temp;
}

bool Heizkreis::isValveOpen() const {
    return _valve_is_open || _state == HeizkreisState::OPENING || 
           _state == HeizkreisState::OPEN;
}

void Heizkreis::setTimeConstant(uint32_t seconds) {
    if (seconds >= TIME_CONSTANT_MIN_SECONDS && 
        seconds <= TIME_CONSTANT_MAX_SECONDS) {
        _time_constant_seconds = seconds;
        Logger::log(TAG, "HK%d: Zeitkonstante = %u Sekunden", _id, seconds);
    }
}

uint32_t Heizkreis::getTimeConstant() const {
    return _time_constant_seconds;
}

void Heizkreis::emergencyStop() {
    Logger::log(TAG, "HK%d: 🚨 NOTFALL-STOPP", _id);
    _state = HeizkreisState::ERROR;
    _valve_is_open = false;
    _heating_power = 0;
    RelayControl::getInstance().setValve(_id, false);
}

uint8_t Heizkreis::getHeatingPower() const {
    return _heating_power;
}
