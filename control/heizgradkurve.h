#pragma once

#include <stdint.h>
#include "config.h"

// Heizgradkurve: Berechnet Vorlauftemperatur basierend auf Außentemperatur
class HeizgradKurve {
public:
    static HeizgradKurve& getInstance();
    
    // Initialisierung der Kurvenparameter
    bool initialize();
    
    // Berechne Sollvorlauftemperatur aus Außentemperatur
    // Implementiert: Linear (Heizkurve)
    float calculateFlowTemperature(float outdoor_temp);
    
    // Setze Kurven-Parameter (für Anpassung zur Laufzeit)
    void setCurveParameters(float outdoor_min, float outdoor_max,
                           float flow_min, float flow_max, float shift);
    
    // Minimale und maximale Vorlauftemperatur
    float getFlowTempMin() const;
    float getFlowTempMax() const;

private:
    HeizgradKurve() = default;
    ~HeizgradKurve() = default;
    HeizgradKurve(const HeizgradKurve&) = delete;
    void operator=(const HeizgradKurve&) = delete;
    
    float _outdoor_min = HEATING_CURVE_OUTDOOR_MIN;
    float _outdoor_max = HEATING_CURVE_OUTDOOR_MAX;
    float _flow_min = HEATING_CURVE_FLOW_MIN;
    float _flow_max = HEATING_CURVE_FLOW_MAX;
    float _shift = HEATING_CURVE_SHIFT;
    bool _initialized = false;
};
