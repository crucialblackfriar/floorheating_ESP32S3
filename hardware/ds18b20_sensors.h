#pragma once

#include <vector>
#include "config.h"

class DS18B20Sensors {
public:
    static DS18B20Sensors& getInstance();
    
    // Initialisierung der 1-Wire Sensoren
    bool initialize();
    
    // Alle Sensoren auslesen (non-blocking)
    void requestTemperatures();
    
    // Temperatur für einen Sensor abrufen (in °C)
    float getTemperature(uint8_t sensor_id);
    
    // Vorlauftemperatur (Sensor 0)
    float getFlowTemperature();
    
    // Rücklauftemperatur für einen Heizkreis (Sensoren 1-6)
    float getReturnTemperature(uint8_t heizkreis_id);
    
    // Alle Sensoren zurücksetzen
    void reset();
    
    // Status-Check: Sind alle Sensoren erreichbar?
    bool allSensorsConnected();

private:
    DS18B20Sensors() = default;
    ~DS18B20Sensors() = default;
    DS18B20Sensors(const DS18B20Sensors&) = delete;
    void operator=(const DS18B20Sensors&) = delete;
    
    bool _initialized = false;
    std::vector<float> _temperatures;
    uint32_t _last_request_time = 0;
};
