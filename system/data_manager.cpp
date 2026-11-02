#include "data_manager.h"
#include "logging/logger.h"
#include "esp_timer.h"
#include <cstring>
#include <cmath>
#include <cstdio>

static const char* TAG = "DataManager";

#define TEMP_SENSOR_TIMEOUT_MS 300000  // 5 minutes
#define MIN_VALID_TEMP -10.0f
#define MAX_VALID_TEMP 80.0f

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

bool DataManager::initialize(uint8_t num_heizkreise) {
    Logger::log(TAG, "Initialisiere DataManager für %d Heizkreise",
                num_heizkreise);
    
    _num_heizkreise = num_heizkreise;
    _circuit_data.resize(num_heizkreise);
    
    for (uint8_t i = 0; i < num_heizkreise; i++) {
        _circuit_data[i].room_temp = 20.0f;
        _circuit_data[i].return_temp = 40.0f;
        _circuit_data[i].is_valid = false;
        _circuit_data[i].last_update_time = 0;
    }
    
    _flow_temperature = 50.0f;
    _outdoor_temperature = 10.0f;
    _last_global_update_time = esp_timer_get_time() / 1000;
    _initialized = true;
    
    Logger::log(TAG, "✓ DataManager initialisiert");
    return true;
}

void DataManager::updateRoomTemperatures(const float* temps) {
    if (!_initialized || !temps) {
        return;
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        if (_isValidTemperatureReading(temps[i])) {
            _circuit_data[i].room_temp = temps[i];
            _circuit_data[i].is_valid = true;
            _circuit_data[i].last_update_time = now;
        }
    }
    
    _last_global_update_time = now;
}

void DataManager::updateReturnTemperatures(const float* temps) {
    if (!_initialized || !temps) {
        return;
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        if (_isValidTemperatureReading(temps[i])) {
            _circuit_data[i].return_temp = temps[i];
            _circuit_data[i].last_update_time = now;
        }
    }
    
    _last_global_update_time = now;
}

void DataManager::updateFlowTemperature(float temp) {
    if (!_initialized || !_isValidTemperatureReading(temp)) {
        return;
    }
    
    _flow_temperature = temp;
    _flow_temp_last_update = esp_timer_get_time() / 1000;
    _last_global_update_time = _flow_temp_last_update;
}

void DataManager::updateOutdoorTemperature(float temp) {
    if (!_initialized || !_isValidTemperatureReading(temp)) {
        return;
    }
    
    _outdoor_temperature = temp;
    _outdoor_temp_last_update = esp_timer_get_time() / 1000;
    _last_global_update_time = _outdoor_temp_last_update;
}

void DataManager::updateValveStates(const bool* states) {
    if (!_initialized || !states) {
        return;
    }
    
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        _circuit_data[i].valve_state = states[i];
    }
}

void DataManager::updateSetpoints(const float* setpoints) {
    if (!_initialized || !setpoints) {
        return;
    }
    
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        if (setpoints[i] >= SETPOINT_MIN_CELSIUS && 
            setpoints[i] <= SETPOINT_MAX_CELSIUS) {
            _circuit_data[i].setpoint = setpoints[i];
        }
    }
}

float DataManager::getRoomTemperature(uint8_t heizkreis_id) const {
    if (heizkreis_id >= _num_heizkreise) {
        return 20.0f;
    }
    return _circuit_data[heizkreis_id].room_temp;
}

float DataManager::getReturnTemperature(uint8_t heizkreis_id) const {
    if (heizkreis_id >= _num_heizkreise) {
        return 40.0f;
    }
    return _circuit_data[heizkreis_id].return_temp;
}

float DataManager::getFlowTemperature() const {
    return _flow_temperature;
}

float DataManager::getOutdoorTemperature() const {
    return _outdoor_temperature;
}

bool DataManager::getValveState(uint8_t heizkreis_id) const {
    if (heizkreis_id >= _num_heizkreise) {
        return false;
    }
    return _circuit_data[heizkreis_id].valve_state;
}

float DataManager::getSetpoint(uint8_t heizkreis_id) const {
    if (heizkreis_id >= _num_heizkreise) {
        return 21.0f;
    }
    return _circuit_data[heizkreis_id].setpoint;
}

uint32_t DataManager::getLastUpdateTime() const {
    return _last_global_update_time;
}

bool DataManager::isRoomTemperatureSensorValid(uint8_t heizkreis_id) const {
    if (heizkreis_id >= _num_heizkreise) {
        return false;
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    uint32_t elapsed = now - _circuit_data[heizkreis_id].last_update_time;
    
    return _circuit_data[heizkreis_id].is_valid && 
           (elapsed < TEMP_SENSOR_TIMEOUT_MS);
}

bool DataManager::isReturnTemperatureSensorValid(uint8_t heizkreis_id) const {
    // Return temperature from DS18B20 should be updated regularly
    if (heizkreis_id >= _num_heizkreise) {
        return false;
    }
    
    uint32_t now = esp_timer_get_time() / 1000;
    uint32_t elapsed = now - _circuit_data[heizkreis_id].last_update_time;
    
    return (elapsed < TEMP_SENSOR_TIMEOUT_MS);
}

bool DataManager::isFlowTemperatureSensorValid() const {
    uint32_t now = esp_timer_get_time() / 1000;
    uint32_t elapsed = now - _flow_temp_last_update;
    
    return (elapsed < TEMP_SENSOR_TIMEOUT_MS);
}

bool DataManager::isOutdoorTemperatureSensorValid() const {
    uint32_t now = esp_timer_get_time() / 1000;
    uint32_t elapsed = now - _outdoor_temp_last_update;
    
    return (elapsed < TEMP_SENSOR_TIMEOUT_MS);
}

bool DataManager::_isValidTemperatureReading(float temp) const {
    return !std::isnan(temp) && !std::isinf(temp) &&
           (temp >= MIN_VALID_TEMP) && (temp <= MAX_VALID_TEMP);
}

const char* DataManager::getSensorDataJson() {
    // Build JSON with current sensor readings
    int offset = 0;
    offset += snprintf(_json_buffer + offset, sizeof(_json_buffer) - offset,
                      "{\"timestamp\":%u,\"circuits\":[",
                      _last_global_update_time);
    
    for (uint8_t i = 0; i < _num_heizkreise; i++) {
        if (i > 0) {
            offset += snprintf(_json_buffer + offset, sizeof(_json_buffer) - offset, ",");
        }
        
        const SensorData& data = _circuit_data[i];
        offset += snprintf(_json_buffer + offset, sizeof(_json_buffer) - offset,
                          "{\"id\":%d,\"room_temp\":%.1f,\"return_temp\":%.1f,"
                          "\"setpoint\":%.1f,\"valve\":%s,\"valid\":%s}",
                          i, data.room_temp, data.return_temp, data.setpoint,
                          data.valve_state ? "true" : "false",
                          isRoomTemperatureSensorValid(i) ? "true" : "false");
    }
    
    offset += snprintf(_json_buffer + offset, sizeof(_json_buffer) - offset,
                      "],\"flow_temp\":%.1f,\"outdoor_temp\":%.1f,"
                      "\"flow_valid\":%s,\"outdoor_valid\":%s}",
                      _flow_temperature, _outdoor_temperature,
                      isFlowTemperatureSensorValid() ? "true" : "false",
                      isOutdoorTemperatureSensorValid() ? "true" : "false");
    
    return _json_buffer;
}
