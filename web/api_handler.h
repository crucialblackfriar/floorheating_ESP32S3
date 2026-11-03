#pragma once

#include <stdint.h>
#include "esp_http_server.h"

// REST API handler for web interface
class APIHandler {
public:
    static APIHandler& getInstance();
    
    // Register API endpoints with HTTP server
    bool registerEndpoints(httpd_handle_t server);
    
    // API response handlers
    static esp_err_t handleGetSensors(httpd_req_t* req);
    static esp_err_t handleGetStatus(httpd_req_t* req);
    static esp_err_t handleSetSetpoint(httpd_req_t* req);
    static esp_err_t handleGetConfig(httpd_req_t* req);

private:
    APIHandler() = default;
    ~APIHandler() = default;
    APIHandler(const APIHandler&) = delete;
    void operator=(const APIHandler&) = delete;
    
    bool _registered = false;
};
