#include "thermal_model.h"
#include "logging/logger.h"
#include <cmath>

static const char* TAG = "ThermalModel";

ThermalModel& ThermalModel::getInstance() {
    static ThermalModel instance;
    return instance;
}

bool ThermalModel::initialize() {
    Logger::log(TAG, "Initialisiere Thermales Modell");
    Logger::log(TAG, "  R_room = %.3f K/W", _R_room);
    Logger::log(TAG, "  C_room = %.0f J/K", _C_room);
    Logger::log(TAG, "  η = %.1f%%", _efficiency * 100);
    Logger::log(TAG, "  P_on = %.0f W, P_off = %.0f W", 
               _heater_power_on, _heater_power_off);
    
    _initialized = true;
    return true;
}

float ThermalModel::predictRoomTemperature(float current_temp, float flow_temp,
                                           bool valve_open, uint32_t delta_
// end is missing
