#include "logger.h"
#include <cstdio>
#include <ctime>
#include "esp_log.h"

uint8_t Logger::_log_level = 1;  // INFO level default
bool Logger::_initialized = false;

bool Logger::initialize() {
    _initialized = true;
    esp_log_level_set("*", ESP_LOG_INFO);
    printf("[LOGGER] System-Logging initialisiert\n");
    return true;
}

void Logger::log(const char* tag, const char* format, ...) {
    if (!_initialized || !tag || !format) {
        return;
    }
    
    // Get current time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Print timestamp and tag
    printf("[%02d:%02d:%02d] [%-15s] ",
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tag);
    
    // Print formatted message
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}

void Logger::setLogLevel(uint8_t level) {
    _log_level = level;
}

void Logger::flush() {
    fflush(stdout);
}
