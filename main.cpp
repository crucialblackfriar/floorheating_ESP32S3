#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "config.h"
#include "logging/logger.h"
#include "system/data_manager.h"
#include "system/nvs_storage.h"
#include "hardware/ds18b20_sensors.h"
#include "hardware/relay_control.h"
#include "wireless/mobile_alerts_receiver.h"
#include "wireless/wifi_manager.h"
#include "web/webserver.h"
#include "control/two_point_controller.h"
#include "control/heizgradkurve.h"
#include "control/predictive_controller.h"
#include "dynamics/time_constant_estimator.h"
#include "dynamics/thermal_model.h"

static const char* TAG = "Main";

// Task handles
static TaskHandle_t sensor_task_handle = nullptr;
static TaskHandle_t control_task_handle = nullptr;
static TaskHandle_t wireless_task_handle = nullptr;

// Forward declarations
static void sensorReadTask(void* pvParameters);
static void controlLoopTask(void* pvParameters);
static void wirelessReceiverTask(void* pvParameters);
static void initializeHardware();
static void initializeSystem();

/**
 * @brief Main application entry point
 */
extern "C" void app_main(void) {
    Logger::log(TAG, "=====================================");
    Logger::log(TAG, "Fußbodenheizungs-Regler v1.0");
    Logger::log(TAG, "ESP32-S3 DevKitC-1 N16R8");
    Logger::log(TAG, "=====================================");
    
    // Initialize NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Logger::log(TAG, "NVS Flash wird zurückgesetzt...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    Logger::log(TAG, "✓ NVS initialisiert");
    
    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    Logger::log(TAG, "✓ Netzwerk-Stack initialisiert");
    
    // Initialize hardware components
    initializeHardware();
    
    // Initialize system components
    initializeSystem();
    
    // Start FreeRTOS tasks
    Logger::log(TAG, "Starte FreeRTOS-Aufgaben");
    
    xTaskCreatePinnedToCore(
        sensorReadTask,
        "SensorRead",
        4096,
        nullptr,
        configMAX_PRIORITIES - 2,
        &sensor_task_handle,
        PRO_CPU_NUM
    );
    
    xTaskCreatePinnedToCore(
        controlLoopTask,
        "ControlLoop",
        8192,
        nullptr,
        configMAX_PRIORITIES - 1,
        &control_task_handle,
        PRO_CPU_NUM
    );
    
    xTaskCreatePinnedToCore(
        wirelessReceiverTask,
        "WirelessRx",
        4096,
        nullptr,
        configMAX_PRIORITIES - 3,
        &wireless_task_handle,
        APP_CPU_NUM
    );
    
    Logger::log(TAG, "✓ Alle Aufgaben gestartet");
    Logger::log(TAG, "=====================================");
    Logger::log(TAG, "System bereit!");
    Logger::log(TAG, "=====================================");
}

/**
 * @brief Initialize all hardware components
 */
static void initializeHardware() {
    Logger::log(TAG, "Initialisiere Hardware-Komponenten");
    
    // Initialize relay control
    if (!RelayControl::getInstance().initialize(NUM_HEIZKREISE, PUMP_RELAY_GPIO)) {
        Logger::log(TAG, "✗ Fehler beim Initialisieren der Relaissteuerung");
    } else {
        Logger::log(TAG, "✓ Relaissteuerung initialisiert");
    }
    
    // Initialize DS18B20 temperature sensors
    if (!DS18B20Sensors::getInstance().initialize(ONE_WIRE_BUS_GPIO)) {
        Logger::log(TAG, "✗ Fehler beim Initialisieren der DS18B20-Sensoren");
    } else {
        Logger::log(TAG, "✓ DS18B20-Sensoren initialisiert");
    }
    
    // Initialize mobile alerts receiver
    if (!MobileAlertsReceiver::getInstance().initialize(MA10200_RECEIVER_GPIO)) {
        Logger::log(TAG, "✗ Fehler beim Initialisieren des Mobile Alerts Empfängers");
    } else {
        Logger::log(TAG, "✓ Mobile Alerts Empfänger initialisiert");
    }
}

/**
 * @brief Initialize all software components
 */
static void initializeSystem() {
    Logger::log(TAG, "Initialisiere Software-Komponenten");
    
    // Initialize DataManager
    if (!DataManager::getInstance().initialize(NUM_HEIZKREISE)) {
        Logger::log(TAG, "✗ Fehler beim Initialisieren des DataManagers");
    } else {
        Logger::log(TAG, "✓ DataManager initialisiert");
    }
    
    // Initialize NVS Storage
    if (!NVSStorage::getInstance().initialize()) {
        Logger::log(TAG, "✗ Fehler beim Initialisieren des NVS-Speichers");
    } else {
        Logger::log(TAG, "✓ NVS-Speicher initialisiert");
    }
    
    // Load persisted setpoints
    float setpoints[NUM_HEIZKREISE];
    NVSStorage::getInstance().loadSetpoints(setpoints, NUM_HEIZKREISE);
    DataManager::getInstance().updateSetpoints(setpoints);
    
    // Initialize heating curve
    Heizgradkurve::getInstance().initialize(
        HEATING_CURVE_OUTDOOR_MIN,
        HEATING_CURVE_OUTDOOR_MAX,
        HEATING_CURVE_FLOW_MIN,
        HEATING_CURVE_FLOW_MAX,
        HEATING_CURVE_SHIFT
    );
    Logger::log(TAG, "✓ Heizgradkurve initialisiert");
    
    // Initialize control system
    TwoPointController::getInstance().initialize(NUM_HEIZKREISE);
    Logger::log(TAG, "✓ Zweipoint-Regler initialisiert");
    
    // Initialize thermal model
    ThermalModel::getInstance().initialize(NUM_HEIZKREISE);
    Logger::log(TAG, "✓ Thermisches Modell initialisiert");
    
    // Initialize time constant estimator
    TimeConstantEstimator::getInstance().initialize(NUM_HEIZKREISE);
    Logger::log(TAG, "✓ Zeitkonstanten-Schätzer initialisiert");
    
    // Initialize WiFi and WebServer
    if (!WiFiManager::getInstance().initialize(WIFI_SSID, WIFI_PASSWORD)) {
        Logger::log(TAG, "✗ Fehler beim Initialisieren von WiFi");
    } else {
        Logger::log(TAG, "✓ WiFi verbunden");
    }
    
    if (!WebServer::getInstance().initialize(WEBSERVER_PORT)) {
        Logger::log(TAG, "✗ Fehler beim Initialisieren des Webservers");
    } else {
        Logger::log(TAG, "✓ Webserver gestartet");
    }
}

/**
 * @brief Sensor reading task
 * Periodically reads DS18B20 sensors and updates DataManager
 */
static void sensorReadTask(void* pvParameters) {
    const TickType_t sensor_read_interval = pdMS_TO_TICKS(2000);  // 2 seconds
    TickType_t last_wake_time = xTaskGetTickCount();
    
    Logger::log(TAG, "Sensor-Lesevorgänge gestartet");
    
    while (true) {
        // Read flow temperature (Vorlauf)
        float flow_temp = DS18B20Sensors::getInstance().readTemperature(0);
        if (!std::isnan(flow_temp)) {
            DataManager::getInstance().updateFlowTemperature(flow_temp);
        }
        
        // Read return temperatures (Rücklauf) for each circuit
        float return_temps[NUM_HEIZKREISE];
        for (uint8_t i = 0; i < NUM_HEIZKREISE; i++) {
            return_temps[i] = DS18B20Sensors::getInstance().readTemperature(i + 1);
        }
        DataManager::getInstance().updateReturnTemperatures(return_temps);
        
        // Read room temperatures from Mobile Alerts receivers
        float room_temps[NUM_HEIZKREISE];
        for (uint8_t i = 0; i < NUM_HEIZKREISE; i++) {
            room_temps[i] = MobileAlertsReceiver::getInstance().getTemperature(i);
        }
        DataManager::getInstance().updateRoomTemperatures(room_temps);
        
        // Log every 30 seconds
        static uint8_t log_counter = 0;
        if (++log_counter >= 15) {  // 15 * 2 seconds = 30 seconds
            Logger::log(TAG, "Vorlauf: %.1f°C, Außen: %.1f°C",
                       flow_temp,
                       DataManager::getInstance().getOutdoorTemperature());
            log_counter = 0;
        }
        
        xTaskDelayUntil(&last_wake_time, sensor_read_interval);
    }
}

/**
 * @brief Main control loop task
 * Implements the heating control algorithm
 */
static void controlLoopTask(void* pvParameters) {
    const TickType_t control_interval = pdMS_TO_TICKS(5000);  // 5 seconds
    TickType_t last_wake_time = xTaskGetTickCount();
    
    Logger::log(TAG, "Hauptregelschleife gestartet");
    
    DataManager& dm = DataManager::getInstance();
    TwoPointController& tpc = TwoPointController::getInstance();
    Heizgradkurve& hgk = Heizgradkurve::getInstance();
    TimeConstantEstimator& tce = TimeConstantEstimator::getInstance();
    
    while (true) {
        // Get current sensor readings
        float outdoor_temp = dm.getOutdoorTemperature();
        float flow_temp = dm.getFlowTemperature();
        
        // Calculate target flow temperature from heating curve
        float target_flow_temp = hgk.calculateFlowTemperature(outdoor_temp);
        
        // Update heating circuit control for each circuit
        for (uint8_t i = 0; i < NUM_HEIZKREISE; i++) {
            float room_temp = dm.getRoomTemperature(i);
            float return_temp = dm.getReturnTemperature(i);
            float setpoint = dm.getSetpoint(i);
            
            // Estimate time constant if return temperature is valid
            if (dm.isReturnTemperatureSensorValid(i)) {
                tce.update(i, return_temp, flow_temp);
            }
            
            // Update two-point controller
            bool valve_state = tpc.update(i, room_temp, setpoint, flow_temp, return_temp);
            
            // Update relay control
            RelayControl::getInstance().setRelayState(i, valve_state);
        }
        
        // Update pump control based on valve states
        bool any_valve_open = false;
        for (uint8_t i = 0; i < NUM_HEIZKREISE; i++) {
            if (dm.getValveState(i)) {
                any_valve_open = true;
                break;
            }
        }
        
        // Pump shutdown delay (30 seconds after last valve close)
        if (any_valve_open) {
            RelayControl::getInstance().setPumpRelay(true);
        } else {
            // Pump will be shut down after PUMP_SHUTDOWN_DELAY_MS
            RelayControl::getInstance().setPumpRelay(false);
        }
        
        // Estimate room temperatures using thermal model (every 30 seconds)
        static uint8_t model_counter = 0;
        if (++model_counter >= 6) {  // 6 * 5 seconds = 30 seconds
            ThermalModel& tm = ThermalModel::getInstance();
            for (uint8_t i = 0; i < NUM_HEIZKREISE; i++) {
                float predicted_temp = tm.predictRoomTemperature(
                    i,
                    dm.getRoomTemperature(i),
                    dm.getSetpoint(i),
                    dm.getValveState(i) ? 100 : 0  // Heating power %
                );
                // Optionally log predictions
            }
            model_counter = 0;
        }
        
        xTaskDelayUntil(&last_wake_time, control_interval);
    }
}

/**
 * @brief Wireless receiver task
 * Processes Mobile Alerts MA 10200 thermometer data
 */
static void wirelessReceiverTask(void* pvParameters) {
    const TickType_t wireless_interval = pdMS_TO_TICKS(1000);  // 1 second
    TickType_t last_wake_time = xTaskGetTickCount();
    
    Logger::log(TAG, "Funkempfänger-Aufgabe gestartet");
    
    while (true) {
        // Process incoming Mobile Alerts data
        MobileAlertsReceiver::getInstance().process();
        
        // Update outdoor temperature if available
        float outdoor_temp = MobileAlertsReceiver::getInstance()
            .getTemperature(MA10200_CHANNEL_OUTDOOR);
        if (!std::isnan(outdoor_temp)) {
            DataManager::getInstance().updateOutdoorTemperature(outdoor_temp);
        }
        
        xTaskDelayUntil(&last_wake_time, wireless_interval);
    }
}
