#pragma once

#include <stdint.h>
#include "config.h"

// Status eines Heizkreises
enum class HeizkreisState {
    IDLE,           // Ventil geschlossen
    OPENING,        // Ventil öffnet (während Totzeit)
    OPEN,           // Ventil offen
    CLOSING,        // Ventil schließt
    ERROR           // Fehler oder Timeout
};

class Heizkreis {
public:
    explicit Heizkreis(uint8_t id);
    ~Heizkreis() = default;
    
    // Initialisierung
    void initialize();
    
    // Regelschleife aktualisieren (wird zyklisch aufgerufen)
    void update(float current_room_temp, float flow_temp, float return_temp);
    
    // Solltemperatur setzen
    void setSetpoint(float setpoint_celsius);
    float getSetpoint() const;
    
    // Status abfragen
    HeizkreisState getState() const;
    float getRoomTemperature() const;
    float getReturnTemperature() const;
    bool isValveOpen() const;
    
    // Zeitkonstante für diesen Kreislauf (Rücklauf-Vorlauf Verzögerung)
    void setTimeConstant(uint32_t seconds);
    uint32_t getTimeConstant() const;
    
    // Notfall: Ventil sofort schließen
    void emergencyStop();
    
    // Heizleistung in % (0-100)
    uint8_t getHeatingPower() const;

private:
    uint8_t _id;
    float _setpoint = 21.0f;
    float _current_room_temp = 20.0f;
    float _current_return_temp = 40.0f;
    float _current_flow_temp = 50.0f;
    
    HeizkreisState _state = HeizkreisState::IDLE;
    bool _valve_is_open = false;
    
    uint32_t _time_constant_seconds = 300;  // Default 5 Minuten
    uint32_t _state_change_time = 0;
    uint8_t _heating_power = 0;
    
    // Intern: Regellogik
    void _updateTwoPointControl(float temp_diff);
    void _checkStateTransitions();
};
