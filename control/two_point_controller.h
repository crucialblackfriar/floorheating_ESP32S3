#pragma once

#include <stdint.h>
#include <vector>
#include "config.h"
#include "heizkreis.h"

// Zweipunkt-Regler mit Hysterese für die innere Regelschleife
class TwoPointController {
public:
    static TwoPointController& getInstance();
    
    // Initialisierung mit allen Heizkreisen
    bool initialize(std::vector<Heizkreis>* heizkreise);
    
    // Regelschleife ausführen (wird zyklisch alle ~100ms aufgerufen)
    void update(float vorlauf_temp, float* return_temps, 
                float outdoor_temp, float* room_temps);
    
    // Pumpe-Management: Schalte Pumpe ab wenn alle Ventile geschlossen
    bool shouldPumpBeOn();
    
    // Debug-Ausgabe
    void printStatus();

private:
    TwoPointController() = default;
    ~TwoPointController() = default;
    TwoPointController(const TwoPointController&) = delete;
    void operator=(const TwoPointController&) = delete;
    
    std::vector<Heizkreis>* _heizkreise = nullptr;
    bool _pump_on = false;
    uint32_t _pump_switch_time = 0;
};
