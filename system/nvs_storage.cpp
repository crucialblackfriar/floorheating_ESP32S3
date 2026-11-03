#include "nvs_storage.h"
#include "logging/logger.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstring>
#include <cstdint>

static const char* TAG = "NVSStorage";

NVSStorage& NVSStorage::getInstance() {
    static NVSStorage instance;
    return instance;
}

bool NVSStorage::initialize() {
    Logger::log(TAG, "Initialisiere NVS-Speicher");
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Logger::log(TAG, "⚠ NVS-Partition löschen und neu initialisieren");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        Logger::log(TAG, "✗ NVS-Initialisierung fehlgeschlagen: %s", 
                   esp_err_to_name(err));
        return false;
    }
    
    _initialized = true;
    Logger::log(TAG, "✓ NVS-Speicher initialisiert");
    return true;
}

bool NVSStorage::saveSetpoints(const float* setpoints, uint8_t num_circuits) {
    if (!_initialized || !setpoints || num_circuits == 0) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        Logger::log(TAG, "✗ NVS-Handle öffnen fehlgeschlagen: %s",
                   esp_err_to_name(err));
        return false;
    }
    
    // Store setpoints as blob
    size_t blob_size = sizeof(float) * num_circuits;
    err = nvs_set_blob(nvs_handle, NVS_KEY_SETPOINTS, 
                       (const void*)setpoints, blob_size);
    
    if (err != ESP_OK) {
        Logger::log(TAG, "✗ Setpoints speichern fehlgeschlagen: %s",
                   esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    
    // Commit changes
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        Logger::log(TAG, "✓ %d Setpoints gespeichert", num_circuits);
        return true;
    } else {
        Logger::log(TAG, "✗ NVS-Commit fehlgeschlagen: %s", esp_err_to_name(err));
        return false;
    }
}

bool NVSStorage::loadSetpoints(float* setpoints, uint8_t num_circuits) {
    if (!_initialized || !setpoints || num_circuits == 0) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        Logger::log(TAG, "ℹ Keine gespeicherten Setpoints vorhanden");
        return false;
    }
    
    size_t blob_size = sizeof(float) * num_circuits;
    err = nvs_get_blob(nvs_handle, NVS_KEY_SETPOINTS, 
                       (void*)setpoints, &blob_size);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK && blob_size == sizeof(float) * num_circuits) {
        Logger::log(TAG, "✓ %d Setpoints geladen", num_circuits);
        return true;
    } else {
        Logger::log(TAG, "✗ Setpoints laden fehlgeschlagen: %s", 
                   esp_err_to_name(err));
        return false;
    }
}

bool NVSStorage::saveTimeConstants(const uint32_t* time_constants, uint8_t num_circuits) {
    if (!_initialized || !time_constants || num_circuits == 0) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t blob_size = sizeof(uint32_t) * num_circuits;
    err = nvs_set_blob(nvs_handle, NVS_KEY_TIME_CONSTANTS, 
                       (const void*)time_constants, blob_size);
    
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return false;
    }
    
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        Logger::log(TAG, "✓ %d Zeitkonstanten gespeichert", num_circuits);
        return true;
    }
    return false;
}

bool NVSStorage::loadTimeConstants(uint32_t* time_constants, uint8_t num_circuits) {
    if (!_initialized || !time_constants || num_circuits == 0) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t blob_size = sizeof(uint32_t) * num_circuits;
    err = nvs_get_blob(nvs_handle, NVS_KEY_TIME_CONSTANTS, 
                       (void*)time_constants, &blob_size);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK && blob_size == sizeof(uint32_t) * num_circuits) {
        Logger::log(TAG, "✓ %d Zeitkonstanten geladen", num_circuits);
        return true;
    }
    return false;
}

bool NVSStorage::saveValveCalibration(const float* calibration_data, uint8_t num_circuits) {
    if (!_initialized || !calibration_data || num_circuits == 0) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t blob_size = sizeof(float) * num_circuits;
    err = nvs_set_blob(nvs_handle, NVS_KEY_VALVE_CALIBRATION, 
                       (const void*)calibration_data, blob_size);
    
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return false;
    }
    
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        Logger::log(TAG, "✓ Ventil-Kalibrierung gespeichert");
        return true;
    }
    return false;
}

bool NVSStorage::loadValveCalibration(float* calibration_data, uint8_t num_circuits) {
    if (!_initialized || !calibration_data || num_circuits == 0) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t blob_size = sizeof(float) * num_circuits;
    err = nvs_get_blob(nvs_handle, NVS_KEY_VALVE_CALIBRATION, 
                       (void*)calibration_data, &blob_size);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK && blob_size == sizeof(float) * num_circuits) {
        Logger::log(TAG, "✓ Ventil-Kalibrierung geladen");
        return true;
    }
    return false;
}

bool NVSStorage::saveThermalParameters(float room_resistance, float room_capacity,
                                       float efficiency) {
    if (!_initialized) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    // Store three float values
    err |= nvs_set_blob(nvs_handle, "thermal_R", &room_resistance, sizeof(float));
    err |= nvs_set_blob(nvs_handle, "thermal_C", &room_capacity, sizeof(float));
    err |= nvs_set_blob(nvs_handle, "thermal_eta", &efficiency, sizeof(float));
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        Logger::log(TAG, "✓ Thermale Parameter gespeichert");
        return true;
    }
    return false;
}

bool NVSStorage::loadThermalParameters(float& room_resistance, float& room_capacity,
                                       float& efficiency) {
    if (!_initialized) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t len = sizeof(float);
    err = nvs_get_blob(nvs_handle, "thermal_R", &room_resistance, &len);
    err |= nvs_get_blob(nvs_handle, "thermal_C", &room_capacity, &len);
    err |= nvs_get_blob(nvs_handle, "thermal_eta", &efficiency, &len);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        Logger::log(TAG, "✓ Thermale Parameter geladen");
        return true;
    }
    return false;
}

bool NVSStorage::clearAll() {
    if (!_initialized) {
        return false;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    err = nvs_erase_all(nvs_handle);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        Logger::log(TAG, "✓ NVS-Speicher geleert");
        return true;
    }
    return false;
}

uint32_t NVSStorage::_calculateChecksum(const uint8_t* data, uint32_t length) const {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < length; i++) {
        checksum += data[i];
        checksum = ((checksum << 1) | (checksum >> 31));  // Rotate left
    }
    return checksum;
}

bool NVSStorage::_validateChecksum(const uint8_t* data, uint32_t length,
                                   uint32_t checksum) const {
    return _calculateChecksum(data, length) == checksum;
}
