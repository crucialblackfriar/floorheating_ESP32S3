#pragma once

#include <stdint.h>
#include <string>
#include "config.h"

class WiFiManager {
public:
    static WiFiManager& getInstance();
    
    // Initialisierung und Verbindung
    bool initialize();
    
    // Verbindung zur SSID herstellen
    bool connect(const char* ssid, const char* password);
    
    // Status abfragen
    bool isConnected();
    
    // IP-Adresse abrufen
    std::string getIpAddress();
    
    // Signal-Stärke in dBm (-30 bis -120)
    int8_t getSignalStrength();
    
    // AP-Modus für Konfiguration (Falls Verbindung fehlschlägt)
    bool startAccessPoint(const char* ssid, const char* password);
    
    // Verbindung trennen
    void disconnect();

private:
    WiFiManager() = default;
    ~WiFiManager() = default;
    WiFiManager(const WiFiManager&) = delete;
    void operator=(const WiFiManager&) = delete;
    
    bool _initialized = false;
    bool _connected = false;
};
