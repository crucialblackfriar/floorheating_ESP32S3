#include "mobile_alerts_receiver.h"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "logger.h"

static const char* TAG = "MobileAlerts";

// UART-Konfiguration für 868 MHz Empfänger
#define UART_NUM UART_NUM_1
#define BAUD_RATE 9600
#define RX_BUF_SIZE 1024

static TaskHandle_t receiver_task = nullptr;

MobileAlertsReceiver& MobileAlertsReceiver::getInstance() {
    static MobileAlertsReceiver instance;
    return instance;
}

bool MobileAlertsReceiver::initialize() {
    Logger::log(TAG, "Initialisiere Mobile Alerts MA 10200 Empfänger");
    
    // UART-Konfiguration
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    
    if (uart_param_config(UART_NUM, &uart_config) != ESP_OK) {
        Logger::log(TAG, "✗ UART-Konfiguration fehlgeschlagen");
        return false;
    }
    
    if (uart_driver_install(UART_NUM, RX_BUF_SIZE, 0, 0, nullptr, 0) != ESP_OK) {
        Logger::log(TAG, "✗ UART-Treiber konnte nicht installiert werden");
        return false;
    }
    
    // Initialisiere Kanal-Daten
    for (int i = 0; i <= MA10200_CHANNEL_OUTDOOR; i++) {
        _channel_data[i] = {
            .temperature = 20.0f,
            .humidity = 50,
            .signal_quality = 0,
            .last_update_ms = 0,
            .is_valid = false
        };
    }
    
    _initialized = true;
    Logger::log(TAG, "✓ Empfänger initialisiert auf UART %d @ %d Baud", 
                UART_NUM, BAUD_RATE);
    return true;
}

bool MobileAlertsReceiver::startReceiver() {
    if (!_initialized) {
        Logger::log(TAG, "✗ Receiver nicht initialisiert");
        return false;
    }
    
    // Starte Empfänger-Task
    xTaskCreate(
        [](void* pvParameters) {
            static_cast<MobileAlertsReceiver*>(pvParameters)->_receiveLoop();
        },
        "mobile_alerts_rx",
        4096,
        this,
        5,  // Priorität
        &receiver_task
    );
    
    Logger::log(TAG, "✓ Empfänger-Task gestartet");
    return true;
}

void MobileAlertsReceiver::_receiveLoop() {
    uint8_t rx_buffer[256];
    
    while (true) {
        // Versuche Daten zu empfangen
        int len = uart_read_bytes(UART_NUM, rx_buffer, sizeof(rx_buffer), 
                                  pdMS_TO_TICKS(100));
        
        if (len > 0) {
            // Parse MA 10200 Frame
            uint8_t channel;
            float temperature;
            uint8_t humidity;
            
            if (_decodeMA10200Frame(rx_buffer, len, channel, temperature, humidity)) {
                if (_channel_data.count(channel)) {
                    _channel_data[channel].temperature = temperature;
                    _channel_data[channel].humidity = humidity;
                    _channel_data[channel].signal_quality = 100;
                    _channel_data[channel].last_update_ms = esp_timer_get_time() / 1000;
                    _channel_data[channel].is_valid = true;
                    
                    Logger::log(TAG, "✓ CH%d: %.1f°C, %d%% RH", 
                                channel, temperature, humidity);
                }
            }
        }
        
        // Prüfe Timeout für alle Kanäle
        uint32_t now = esp_timer_get_time() / 1000;
        for (auto& [channel, data] : _channel_data) {
            if (data.last_update_ms > 0 && 
                (now - data.last_update_ms) > MA10200_UPDATE_TIMEOUT_MS) {
                data.is_valid = false;
                Logger::log(TAG, "⚠ CH%d: Timeout (%.1f Sekunden ohne Update)", 
                            channel, (now - data.last_update_ms) / 1000.0f);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

bool MobileAlertsReceiver::_decodeMA10200Frame(const uint8_t* frame, size_t len,
                                                uint8_t& channel, float& temp, 
                                                uint8_t& hum) {
    if (len < 12) return false;
    
    // MA 10200 Protokoll: 
    // [Sync: 0xC0] [Channel+ID: 1 Byte] [Temp: 2 Bytes] [Hum: 1 Byte] [Checksum: 1 Byte]
    
    if (frame[0] != 0xC0) return false;
    
    // Extrahiere Kanal (Bits 5-7)
    channel = (frame[1] >> 5) & 0x07;
    
    // Temperatur: 12-Bit Big-Endian Wert (-40...+60°C)
    int16_t temp_raw = ((frame[2] << 8) | frame[3]);
    temp_raw = (temp_raw >> 4) & 0x0FFF;  // Nur 12 Bit
    
    // Konvertiere zu °C (Auflösung: 0,1°C)
    if (temp_raw & 0x0800) {
        // Negativ (Zweierkomplement)
        temp = -((~temp_raw & 0x0FFF) + 1) * 0.1f;
    } else {
        temp = temp_raw * 0.1f - 40.0f;
    }
    
    // Luftfeuchte
    hum = frame[4];
    
    // Einfache Checksum-Prüfung
    uint8_t checksum = 0;
    for (size_t i = 1; i < len - 1; i++) {
        checksum ^= frame[i];
    }
    
    if (checksum != frame[len - 1]) {
        Logger::log(TAG, "✗ Checksum-Fehler: %02X vs %02X", checksum, frame[len - 1]);
        return false;
    }
    
    return true;
}

MobileAlertsData MobileAlertsReceiver::getChannelData(uint8_t channel_id) {
    if (_channel_data.count(channel_id)) {
        return _channel_data[channel_id];
    }
    return {0.0f, 0, 0, 0, false};
}

float MobileAlertsReceiver::getOutdoorTemperature() {
    auto data = getChannelData(MA10200_CHANNEL_OUTDOOR);
    return data.is_valid ? data.temperature : 20.0f;  // Fallback
}

float MobileAlertsReceiver::getRoomTemperature(uint8_t heizkreis_id) {
    if (heizkreis_id >= NUM_HEIZKREISE) return 20.0f;
    
    // Kanäle 0-5 = Heizkreise 0-5
    auto data = getChannelData(heizkreis_id);
    return data.is_valid ? data.temperature : 20.0f;
}

void MobileAlertsReceiver::update() {
    // Diese Methode wird periodisch aufgerufen, falls nötig
    // Die _receiveLoop läuft bereits im Background
}

bool MobileAlertsReceiver::allSensorsActive() {
    for (int i = 0; i < NUM_HEIZKREISE + 1; i++) {  // +1 für Außensensor
        uint8_t channel = i;
        if (!_channel_data[channel].is_valid) {
            return false;
        }
    }
    return true;
}
