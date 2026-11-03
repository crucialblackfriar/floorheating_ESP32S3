#pragma once

#include <stdint.h>
#include "esp_http_server.h"

// HTTP webserver for ESP32 floor heating controller
// Serves REST API and static web interface
class WebServer {
public:
    static WebServer& getInstance();
    
    // Initialize and start HTTP server
    bool initialize(uint16_t port = 80);
    
    // Stop server and cleanup
    void shutdown();
    
    // Check if server is running
    bool isRunning() const;
    
    // Serve static assets (HTML, CSS, JS)
    void registerStaticAssets();

private:
    WebServer() = default;
    ~WebServer() = default;
    WebServer(const WebServer&) = delete;
    void operator=(const WebServer&) = delete;
    
    httpd_handle_t _server_handle = nullptr;
    bool _running = false;
    uint16_t _port = 80;
};
