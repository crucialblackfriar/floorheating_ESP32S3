#pragma once

#include <stdint.h>
#include <vector>
#include <deque>
#include "config.h"

// Schätzt die thermische Zeitkonstante jedes Heizkreises durch Messung
// der Rücklauftemperatur nach Ventilöffnung/-schließung
class TimeConstantEstimator {
public:
    static TimeConstantEstimator& getInstance();
    
    // Initialisierung
    bool initialize(uint8_t num_heizkreise);
    
    // Aktualisiere mit aktuellem Rücklauf-Temperatur-Vektor
    void update(const float* return_temps, const bool* valve_states);
    
    // Abrufen der geschätzten Zeitkonstante für einen Heizkreis
    uint32_t getTimeConstant(uint8_t heizkreis_id);
    
    // Ruhe-Phase nach Ventilstellungsänderung (nötig für genaue Schätzung)
    bool isEstimationReady(uint8_t heizkreis_id);
    
    // Zurücksetzen einer Schätzung (z.B. bei Fehlererkennung)
    void resetEstimation(uint8_t heizkreis_id);

private:
    TimeConstantEstimator() = default;
    ~TimeConstantEstimator() = default;
    TimeConstantEstimator(const TimeConstantEstimator&) = delete;
    void operator=(const TimeConstantEstimator&) = delete;
    
    struct EstimationState {
        std::deque<float> return_temp_history;  // Rücklauf-Temperatur Verlauf
        uint32_t last_valve_change_time = 0;    // Zeit der letzten Ventilstörung
        bool valve_was_open = false;             // Vorheriger Ventil-State
        uint32_t estimated_time_constant = 300; // Geschätzte τ in Sekunden
        bool estimation_valid = false;           // Ist Schätzung zuverlässig?
        uint32_t samples_collected = 0;          // Anzahl Messwerte gesammelt
    };
    
    std::vector<EstimationState> _estimations;
    uint32_t _last_update_time = 0;
    bool _initialized = false;
    
    // Interne Schätzung
    void _estimateTimeConstant(uint8_t heizkreis_id, float current_return_temp);
};
