#include "heizgradkurve.h"
#include "logging/logger.h"
#include <algorithm>
#include <cmath>

static const char* TAG = "HeizgradKurve";

HeizgradKurve& HeizgradKurve::getInstance() {
    static HeizgradKurve instance;
    return instance;
}

bool HeizgradKurve::initialize(float outdoor_min, float outdoor_max,
                               float flow_min, float flow_max,
                               float shift) {
    Logger::log(TAG, "Initialisiere Heizkurve");
    Logger::log(TAG, "  Außentemp: %.1f - %.1f °C", outdoor_min, outdoor_max);
    Logger::log(TAG, "  Vorlauftemp: %.1f - %.1f °C", flow_min, flow_max);
    Logger::log(TAG, "  Verschiebung: %.2f", shift);
    
    setParameters(outdoor_min, outdoor_max, flow_min, flow_max, shift);
    _initialized = true;
    
    Logger::log(TAG, "✓ Heizkurve initialisiert");
    return true;
}

float HeizgradKurve::calculateFlowTemperature(float outdoor_temp) {
    if (!_initialized) {
        return 50.0f;  // Default fallback
    }
    
    // Clamp outdoor temperature to curve range
    float clamped_outdoor = std::max(_outdoor_min, 
                                     std::min(_outdoor_max, outdoor_temp));
    
    // Linear interpolation with shift
    // As outdoor temperature increases, flow temperature decreases
    float normalized_outdoor = (clamped_outdoor - _outdoor_min) / 
                               (_outdoor_max - _outdoor_min);
    
    // Apply shift (negative shift = more aggressive heating)
    normalized_outdoor = std::max(0.0f, std::min(1.0f, 
                                  normalized_outdoor + _shift));
    
    // Interpolate flow temperature (inverted: cold outdoor = high flow temp)
    float flow_temp = _flow_max - (normalized_outdoor * 
                     (_flow_max - _flow_min));
    
    return flow_temp;
}

void HeizgradKurve::setParameters(float outdoor_min, float outdoor_max,
                                 float flow_min, float flow_max,
                                 float shift) {
    _outdoor_min = outdoor_min;
    _outdoor_max = outdoor_max;
    _flow_min = flow_min;
    _flow_max = flow_max;
    _shift = shift;
    
    Logger::log(TAG, "✓ Heizkurven-Parameter aktualisiert");
}

void HeizgradKurve::getParameters(float& outdoor_min, float& outdoor_max,
                                 float& flow_min, float& flow_max,
                                 float& shift) const {
    outdoor_min = _outdoor_min;
    outdoor_max = _outdoor_max;
    flow_min = _flow_min;
    flow_max = _flow_max;
    shift = _shift;
}
