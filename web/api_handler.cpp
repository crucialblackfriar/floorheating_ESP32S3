#include "api_handler.h"
#include "logging/logger.h"
#include "system/data_manager.h"
#include "system/nvs_storage.h"
#include "control/two_point_controller.h"
#include "control/heizgradkurve.h"
#include <cstring>
#include <cstdio>
#include <cjson/cJSON.h>

static const char* TAG = "APIHandler";

APIHandler& APIHandler::getInstance() {
    static APIHandler instance;
    return instance;
}

bool APIHandler::registerEndpoints(httpd_handle_t server) {
    if (!server || _registered) {
        return false;
    }
    
    Logger::log(TAG, "Registriere API-Endpoints");
    
    // GET /api/sensors - Read sensor values
    httpd_uri_t get_sensors = {
        .uri = "/api/sensors",
        .method = HTTP_GET,
        .handler = handleGetSensors,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &get_sensors);
    
    // GET /api/status - System status
    httpd_uri_t get_status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = handleGetStatus,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &get_status);
    
    // POST /api/setpoint - Set room target temperature
    httpd_uri_t set_setpoint = {
        .uri = "/api/setpoint",
        .method = HTTP_POST,
        .handler = handleSetSetpoint,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &set_setpoint);
    
    // GET /api/config - Get configuration parameters
    httpd_uri_t get_config = {
        .uri = "/api/config",
        .method = HTTP_GET,
        .handler = handleGetConfig,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &get_config);
    
    _registered = true;
    Logger::log(TAG, "✓ %d API-Endpoints registriert", 4);
    
    return true;
}

esp_err_t APIHandler::handleGetSensors(httpd_req_t* req) {
    DataManager& dm = DataManager::getInstance();
    
    const char* json_response = dm.getSensorDataJson();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_response, std::strlen(json_response));
    
    return ESP_OK;
}

esp_err_t APIHandler::handleGetStatus(httpd_req_t* req) {
    DataManager& dm = DataManager::getInstance();
    TwoPointController& tpc = TwoPointController::getInstance();
    
    // Build comprehensive status JSON
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON allocation failed");
        return ESP_OK;
    }
    
    // System status
    cJSON_AddNumberToObject(root, "timestamp", dm.getLastUpdateTime());
    cJSON_AddBoolToObject(root, "flow_sensor_valid", dm.isFlowTemperatureSensorValid());
    cJSON_AddBoolToObject(root, "outdoor_sensor_valid", dm.isOutdoorTemperatureSensorValid());
    cJSON_AddNumberToObject(root, "flow_temperature", dm.getFlowTemperature());
    cJSON_AddNumberToObject(root, "outdoor_temperature", dm.getOutdoorTemperature());
    
    // Heizgradkurve status
    Heizgradkurve& hgk = Heizgradkurve::getInstance();
    cJSON_AddNumberToObject(root, "target_flow_temperature", 
                           hgk.calculateFlowTemperature(dm.getOutdoorTemperature()));
    
    // Circuit status array
    cJSON* circuits = cJSON_CreateArray();
    for (uint8_t i = 0; i < 6; i++) {  // NUM_HEIZKREISE hardcoded as 6 per your config
        cJSON* circuit = cJSON_CreateObject();
        
        cJSON_AddNumberToObject(circuit, "id", i);
        cJSON_AddNumberToObject(circuit, "room_temperature", dm.getRoomTemperature(i));
        cJSON_AddNumberToObject(circuit, "return_temperature", dm.getReturnTemperature(i));
        cJSON_AddNumberToObject(circuit, "setpoint", dm.getSetpoint(i));
        cJSON_AddBoolToObject(circuit, "valve_open", dm.getValveState(i));
        cJSON_AddBoolToObject(circuit, "room_sensor_valid", 
                            dm.isRoomTemperatureSensorValid(i));
        cJSON_AddBoolToObject(circuit, "return_sensor_valid", 
                            dm.isReturnTemperatureSensorValid(i));
        
        // Temperature difference
        float temp_diff = dm.getRoomTemperature(i) - dm.getSetpoint(i);
        cJSON_AddNumberToObject(circuit, "temperature_difference", temp_diff);
        
        cJSON_AddItemToArray(circuits, circuit);
    }
    
    cJSON_AddItemToObject(root, "circuits", circuits);
    
    // Convert to string
    char* json_str = cJSON_Print(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_str, std::strlen(json_str));
    
    free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

esp_err_t APIHandler::handleSetSetpoint(httpd_req_t* req) {
    DataManager& dm = DataManager::getInstance();
    NVSStorage& nvs = NVSStorage::getInstance();
    
    // Read POST body
    char content[256];
    int recv_size = httpd_req_recv(req, content, sizeof(content) - 1);
    
    if (recv_size <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
        return ESP_OK;
    }
    
    content[recv_size] = '\0';
    
    // Parse JSON
    cJSON* json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON parse error");
        return ESP_OK;
    }
    
    // Extract circuit ID and setpoint value
    cJSON* circuit_id_obj = cJSON_GetObjectItem(json, "circuit_id");
    cJSON* setpoint_obj = cJSON_GetObjectItem(json, "setpoint");
    
    if (!circuit_id_obj || !setpoint_obj || 
        !cJSON_IsNumber(circuit_id_obj) || !cJSON_IsNumber(setpoint_obj)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid parameters");
        cJSON_Delete(json);
        return ESP_OK;
    }
    
    uint8_t circuit_id = (uint8_t)circuit_id_obj->valuedouble;
    float setpoint = (float)setpoint_obj->valuedouble;
    
    // Validate range
    if (setpoint < SETPOINT_MIN_CELSIUS || setpoint > SETPOINT_MAX_CELSIUS || 
        circuit_id >= NUM_HEIZKREISE) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Setpoint out of range");
        cJSON_Delete(json);
        return ESP_OK;
    }
    
    // Update setpoint in DataManager
    float setpoints[NUM_HEIZKREISE];
    for (uint8_t i = 0; i < NUM_HEIZKREISE; i++) {
        setpoints[i] = dm.getSetpoint(i);
    }
    setpoints[circuit_id] = setpoint;
    dm.updateSetpoints(setpoints);
    
    // Persist to NVS
    nvs.saveSetpoints(setpoints, NUM_HEIZKREISE);
    
    Logger::log(TAG, "✓ Solltemperatur Heizkreis %d auf %.1f°C gesetzt", 
               circuit_id, setpoint);
    
    // Send response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "circuit_id", circuit_id);
    cJSON_AddNumberToObject(response, "setpoint", setpoint);
    cJSON_AddBoolToObject(response, "success", true);
    
    char* response_str = cJSON_Print(response);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response_str, std::strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

esp_err_t APIHandler::handleGetConfig(httpd_req_t* req) {
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON allocation failed");
        return ESP_OK;
    }
    
    // Hardware configuration
    cJSON_AddNumberToObject(root, "num_heizkreise", NUM_HEIZKREISE);
    cJSON_AddNumberToObject(root, "num_rucklauf_sensoren", NUM_RÜCKLAUF_SENSOREN);
    
    // Control parameters
    cJSON_AddNumberToObject(root, "setpoint_min", SETPOINT_MIN_CELSIUS);
    cJSON_AddNumberToObject(root, "setpoint_max", SETPOINT_MAX_CELSIUS);
    cJSON_AddNumberToObject(root, "hysteresis_room_temp", HYSTERESIS_ROOM_TEMP);
    cJSON_AddNumberToObject(root, "valve_dead_time_ms", VALVE_DEAD_TIME_MS);
    cJSON_AddNumberToObject(root, "pump_shutdown_delay_ms", PUMP_SHUTDOWN_DELAY_MS);
    
    // Heating curve parameters
    cJSON* hgk_config = cJSON_CreateObject();
    cJSON_AddNumberToObject(hgk_config, "outdoor_min", HEATING_CURVE_OUTDOOR_MIN);
    cJSON_AddNumberToObject(hgk_config, "outdoor_max", HEATING_CURVE_OUTDOOR_MAX);
    cJSON_AddNumberToObject(hgk_config, "flow_min", HEATING_CURVE_FLOW_MIN);
    cJSON_AddNumberToObject(hgk_config, "flow_max", HEATING_CURVE_FLOW_MAX);
    cJSON_AddNumberToObject(hgk_config, "shift", HEATING_CURVE_SHIFT);
    cJSON_AddItemToObject(root, "heating_curve", hgk_config);
    
    // Time constant estimation
    cJSON* tc_config = cJSON_CreateObject();
    cJSON_AddNumberToObject(tc_config, "estimation_interval_ms", 
                          TIME_CONSTANT_ESTIMATION_INTERVAL_MS);
    cJSON_AddNumberToObject(tc_config, "min_seconds", TIME_CONSTANT_MIN_SECONDS);
    cJSON_AddNumberToObject(tc_config, "max_seconds", TIME_CONSTANT_MAX_SECONDS);
    cJSON_AddItemToObject(root, "time_constant_estimation", tc_config);
    
    // Convert to string
    char* json_str = cJSON_Print(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_str, std::strlen(json_str));
    
    free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}
