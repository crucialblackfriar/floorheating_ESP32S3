#include "webserver.h"
#include "api_handler.h"
#include "logging/logger.h"
#include <cstring>

static const char* TAG = "WebServer";

WebServer& WebServer::getInstance() {
    static WebServer instance;
    return instance;
}

bool WebServer::initialize(uint16_t port) {
    if (_running) {
        Logger::log(TAG, "⚠ Webserver läuft bereits");
        return false;
    }
    
    Logger::log(TAG, "Starte HTTP-Webserver auf Port %d", port);
    _port = port;
    
    // HTTP server configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.task_priority = tskIDLE_PRIORITY + 1;
    config.stack_size = 8192;
    config.max_uri_handlers = 16;
    
    // Start HTTP server
    esp_err_t ret = httpd_start(&_server_handle, &config);
    if (ret != ESP_OK) {
        Logger::log(TAG, "✗ Fehler beim Starten des Webservers: 0x%x", ret);
        return false;
    }
    
    _running = true;
    Logger::log(TAG, "✓ Webserver gestartet auf http://localhost:%d", port);
    
    // Register API endpoints
    if (!APIHandler::getInstance().registerEndpoints(_server_handle)) {
        Logger::log(TAG, "⚠ Fehler beim Registrieren von API-Endpoints");
        return false;
    }
    
    // Register static assets
    registerStaticAssets();
    
    return true;
}

void WebServer::shutdown() {
    if (_running && _server_handle) {
        Logger::log(TAG, "Fahre Webserver herunter");
        httpd_stop(_server_handle);
        _server_handle = nullptr;
        _running = false;
        Logger::log(TAG, "✓ Webserver gestoppt");
    }
}

bool WebServer::isRunning() const {
    return _running;
}

void WebServer::registerStaticAssets() {
    // Static file handler for index.html
    static httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = [](httpd_req_t* req) {
            extern const uint8_t index_html_start[] asm("_binary_index_html_start");
            extern const uint8_t index_html_end[] asm("_binary_index_html_end");
            
            httpd_resp_set_type(req, "text/html; charset=utf-8");
            httpd_resp_send(req, (const char*)index_html_start,
                          index_html_end - index_html_start);
            return ESP_OK;
        },
        .user_ctx = nullptr
    };
    
    httpd_register_uri_handler(_server_handle, &root_uri);
    
    // Redirect /index.html to /
    static httpd_uri_t index_uri = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = [](httpd_req_t* req) {
            httpd_resp_set_status(req, "301 Moved Permanently");
            httpd_resp_set_hdr(req, "Location", "/");
            httpd_resp_send(req, nullptr, 0);
            return ESP_OK;
        },
        .user_ctx = nullptr
    };
    
    httpd_register_uri_handler(_server_handle, &index_uri);
    
    Logger::log(TAG, "✓ Statische Assets registriert");
}
