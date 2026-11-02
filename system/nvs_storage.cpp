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
    err
// end is missing
