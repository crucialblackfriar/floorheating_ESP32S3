#pragma once

#include <stdint.h>
#include <cstdarg>

// Simple logging system with TAG support
class Logger {
public:
    // Initialize logging
    static bool initialize();
    
    // Log message with TAG
    static void log(const char* tag, const char* format, ...);
    
    // Set log level (DEBUG, INFO, WARNING, ERROR)
    static void setLogLevel(uint8_t level);
    
    // Flush any pending output
    static void flush();

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;
    
    static uint8_t _log_level;
    static bool _initialized;
};
