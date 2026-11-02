#pragma once

#include <stdint.h>
#include <vector>
#include "config.h"

class RelayControl {
public:
    static RelayControl& getInstance();
    
    // Initialisierung
    bool initialize();
    
    // Heizkreis-Ventil steuern (true = öffnen, false = schließen)
    void setValve(uint8_t heizkreis_id, bool open);
    
    // Pumpenschalter steuern
    void setPump(bool on);
    
    // Alle Ventile schließen (Notfall)
    void emergencyShutdown();
    
    // Status abfragen
    bool getValveState(uint8_t heizkreis_id);
    bool getPumpState();
    
    // Für Zyklenbetrieb (Impulssteuerung der Ventile)
    void pulseValve(uint8_t heizkreis_id, uint32_t duration_ms);

private:
    RelayControl() = default;
    ~RelayControl() = default;
    RelayControl(const RelayControl&) = delete;
    void operator=(const RelayControl&) = delete;
    
    bool _initialized = false;
    std::vector<bool> _valve_states;
    bool _pump_state = false;
    std::vector<uint32_t> _valve_pulse_end_time;
};
