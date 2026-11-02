#pragma once

#include <stdint.h>
#include <map>
#include <vector>
#include "config.h"

// Struktur für empfangene Funkdaten
struct MobileAlertsData {
    float temperature;      // in °C
    uint8_t humidity;       // in %
    uint8_t signal_quality; // 0-100%
    uint32_t last_update_ms;// Zeitstempel letztes Update
    bool is_valid;         // Gültig/Timeout
};

class MobileAlertsReceiver {
public:
    static MobileAlertsReceiver& getInstance();
    
    // Initialisierung des 868 MHz Empfängers
    bool initialize();
    
    // Starte Empfangs-Loop (läuft in eigenem Task)
    bool startReceiver();
    
    // Daten für einen Kanal abrufen
    MobileAlertsData getChannelData(uint8_t channel_id);
    
    // Außentemperatur abrufen
    float getOutdoorTemperature();
    
    // Raumtemperatur für einen Heizkreis abrufen
    float getRoomTemperature(uint8_t heizkreis_id);
    
    // Alle Kanäle aktualisieren (wird zyklisch aufgerufen)
    void update();
    
    // Prüfe ob alle Sensoren Daten senden
    bool allSensorsActive();

private:
    MobileAlertsReceiver() = default;
    ~MobileAlertsReceiver() = default;
    MobileAlertsReceiver(const MobileAlertsReceiver&) = delete;
    void operator=(const MobileAlertsReceiver&) = delete;
    
    bool _initialized = false;
    std::map<uint8_t, MobileAlertsData> _channel_data;  // Channel-ID -> Daten
    
    // Interne Receiverlogik
    void _receiveLoop();
    bool _decodeMA10200Frame(const uint8_t* frame, size_t len, 
                             uint8_t& channel, float& temp, uint8_t& hum);
};
