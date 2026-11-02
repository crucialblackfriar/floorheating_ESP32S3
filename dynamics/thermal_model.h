#pragma once

#include <stdint.h>
#include "config.h"

// Thermisches Modell der Fußbodenheizung für Vorhersagen
class ThermalModel {
public:
    static ThermalModel& getInstance();
    
    // Initialisierung mit thermischen Parametern
    bool initialize();
    
    // Prädiktive Vorhersage: Raumtemperatur nach Zeit Δt
    // current_temp: Aktuelle Raumtemperatur [°C]
    // flow_temp: Vorlauftemperatur [°C]
    // valve_open: Ventil offen (true) oder geschlossen (false)
    // delta_time_ms: Vorhersage-Horizont [ms]
    float predictRoomTemperature(float current_temp, float flow_temp, 
                                 bool valve_open, uint32_t delta_time_ms);
    
    // Setze thermische Parameter zur Laufzeit
    void setThermalParameters(float room_thermal_resistance,      // K/W
                              float room_thermal_capacity,         // J/K
                              float floor_efficiency);              // 0...1

private:
    ThermalModel() = default;
    ~ThermalModel() = default;
    ThermalModel(const ThermalModel&) = delete;
    void operator=(const ThermalModel&) = delete;
    
    // Thermische Parameter
    float _R_room = 0.1f;          // Wärmewiderstand Raum [K/W]
    float _C_room = 50000.0f;      // Wärmekapazität Raum [J/K]
    float _efficiency = 0.85f;      // Wirkungsgrad Fußbodenheizung
    
    float _heater_power_on = 8000.0f;   // Heizleistung bei Ventil offen [W]
    float _heater_power_off = -200.0f;  // "Leistung" bei Ventil geschlossen (Abkühlung) [W]
    
    bool _initialized = false;
};
