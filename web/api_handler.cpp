#include "api_handler.h"
#include "logging/logger.h"
#include "system/data_manager.h"
#include "control/two_point_controller.h"
#include <cstring>
#include <cstdio>

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
    httpd_uri_t get_status =
// rest is missing
