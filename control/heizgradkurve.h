#pragma once

#include <stdint.h>

// Heating curve (Heizkurve) implementation
// Maps outdoor temperature to flow (supply) temperature using linear interpolation
class HeizgradKurve {
public:
    static HeizgradKurve& getInstance();
    
    // Initialize with curve parameters
    // outdoor_min/max: outdoor temperature range [°C]
    // flow_min/max: corresponding flow temperature range [°C]
    // shift: curve shift parameter (affects aggressiveness)
    bool initialize(float outdoor_min, float outdoor_max,
                   float flow_min, float flow_max,
                   float shift);
    
    // Calculate target flow temperature based on outdoor temperature
    // outdoor_temp: current outdoor temperature [°C]
    // Returns: target flow temperature [°C]
    float calculateFlowTemperature(float outdoor_temp);
    
    // Set curve parameters at runtime
    void setParameters(float outdoor_min, float outdoor_max,
                      float flow_min, float flow_max,
                      float shift);
    
    // Get current curve parameters
    void getParameters(float& outdoor_min, float& outdoor_max,
                      float& flow_min, float& flow_max,
                      float& shift) const;

private:
    HeizgradKurve() = default;
    ~HeizgradKurve() = default;
    HeizgradKurve(const HeizgradKurve&) = delete;
    void operator=(const HeizgradKurve&) = delete;
    
    float _outdoor_min = -15.0f;
    float _outdoor_max = 20.0f;
    float _flow_min = 20.0f;
    float _flow_max = 60.0f;
    float _shift = 0.5f;
    bool _initialized = false;
};
