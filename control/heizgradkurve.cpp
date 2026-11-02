#include "heizgradkurve.h"
#include "logging/logger.h"
#include <algorithm>
#include <cmath>

static const char* TAG = "HeizgradKurve";

HeizgradKurve& HeizgradKurve::getInstance() {
    static HeizgradKurve instance;
    return instance;
}

bool HeizgradKurve::initialize() {
    Logger::log(TAG, "Initialisiere Heizgradkurve");
    Logger::log(TAG, "  Outdoor: %.1f...%.1f°C", _outdoor_min, _outdoor_max);
    Logger::log(TAG, "  Flow:    %.1f...%.1f°C", _flow_min, _flow_max);
    Logger::log(TAG, "  Shift:   %.2f", _shift);
    
    _initialized = true;
    return true;
}

float HeizgradKurve::calculateFlowTemperature(float outdoor_temp) {
    if (!_initialized) return 45.0f;  // Fallback
    
    // Begrenze Außentemperatur auf Kurvenbereich
    outdoor_temp = std::max(_outdoor_min, std::min(_outdoor_max, outdoor_temp));
    
    // Lineare Interpolation: Je kälter außen, desto heißer Vorlauf
    // Normalisiere Außentemp auf 0...1
    float t_norm = (outdoor_temp - _outdoor_min) / (_outdoor_max - _outdoor_min);
    
    // Invertiert: t_norm = 1 bei Außen_min (kalt), t_norm = 0 bei Außen_max (warm)
    t_norm = 1.0f - t_norm;
    
    // Berechne Vorlauftemperatur mit Verschiebung
    float flow_temp = _flow_min + t_norm * (_flow_max - _flow_min);
    
    // Appliziere Shift-Faktor (Feinabstimmung)
    flow_temp += _shift;
    
    // Begrenze auf zulässigen Bereich
    flow_temp = std::max(_flow_min, std::min(_flow_max + _shift, flow_temp));
    
    // Sicherheit: Maximale Vorlauftemperatur nicht überschreiten
    if (flow_temp > THERMAL_OVERLOAD_THRESHOLD) {
        Logger::log(TAG, "⚠ Thermale Überbelastung: %.1f°C > %.1f°C",
                   flow_temp, (float)THERMAL_OVERLOAD_THRESHOLD);
        flow_temp = THERMAL_OVERLOAD_THRESHOLD;
    }
    
    return flow_temp;
}

void HeizgradKurve::setCurveParameters(float outdoor_min, float outdoor_max,
                                      float flow_min, float flow_max, float shift) {
    if (outdoor_min >= outdoor_max || flow_min >= flow_max) {
        Logger::log(TAG, "✗ Ungültige Kurvenparameter");
        return;
    }
    
    _outdoor_min = outdoor_min;
    _outdoor_max = outdoor_max;
    _flow_min = flow_min;
    _flow_max = flow_max;
    _shift = shift;
    
    Logger::log(TAG, "✓ Kurvenparameter aktualisiert");
    Logger::log(TAG, "  Outdoor: %.1f...%.1f°C", _outdoor_min, _outdoor_max);
    Logger::log(TAG, "  Flow:    %.1f...%.1f°C", _flow_min, _flow_max);
    Logger::log(TAG, "  Shift:   %.2f", _shift);
}

float HeizgradKurve::getFlowTempMin() const {
    return _flow_min;
}

float HeizgradKurve::getFlowTempMax() const {
    return _flow_max;
}
