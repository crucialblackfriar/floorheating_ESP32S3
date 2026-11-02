#include "ds18b20_sensors.h"
#include "pin_definitions.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "esp_log.h"
#include "logger.h"

static const char* TAG = "DS18B20";
static OneWire oneWire(ONE_WIRE_BUS_GPIO);
static DallasTemperature sensors(&oneWire);

DS18B20Sensors& DS18B20Sensors::getInstance() {
    static DS18B20Sensors instance;
    return instance;
}

bool DS18B20Sensors::initialize() {
    Logger::log(TAG, "Initialisiere DS18B20 Sensoren auf GPIO %d", ONE_WIRE_BUS_GPIO);
    
    sensors.begin();
    _temperatures.resize(NUM_HEIZKREISE + 1, 0.0f);  // +1 für Vorlauf
    
    // Setze Auflösung auf 12 Bit (0,0625°C Genauigkeit)
    sensors.setResolution(12);
    
    // Starte asynchrone Messung
    requestTemperatures();
    
    if (allSensorsConnected()) {
        _initialized = true;
        Logger::log(TAG, "✓ %d Sensoren gefunden", sensors.getDeviceCount());
        return true;
    }
    
    Logger::log(TAG, "✗ Fehler: Nur %d Sensoren gefunden (erwartet %d)",
                sensors.getDeviceCount(), NUM_HEIZKREISE + 1);
    return false;
}

void DS18B20Sensors::requestTemperatures() {
    if (!_initialized) return;
    
    // Non-blocking Anfrage mit 750ms Wartezeit für 12-Bit Auflösung
    sensors.requestTemperatures();
    _last_request_time = esp_timer_get_time() / 1000;
}

float DS18B20Sensors::getTemperature(uint8_t sensor_id) {
    if (!_initialized || sensor_id >= _temperatures.size()) {
        return -127.0f;  // Error-Wert
    }
    return _temperatures[sensor_id];
}

float DS18B20Sensors::getFlowTemperature() {
    return getTemperature(0);  // Vorlauf ist Sensor 0
}

float DS18B20Sensors::getReturnTemperature(uint8_t heizkreis_id) {
    if (heizkreis_id >= NUM_HEIZKREISE) return -127.0f;
    return getTemperature(heizkreis_id + 1);  // Rücklauf ab Sensor 1
}

bool DS18B20Sensors::allSensorsConnected() {
    return sensors.getDeviceCount() >= (NUM_HEIZKREISE + 1);
}

void DS18B20Sensors::reset() {
    sensors.begin();
    for (auto& temp : _temperatures) {
        temp = -127.0f;
    }
}
