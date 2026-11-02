#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "logging/logger.h"
#include <cstring>

static const char* TAG = "WiFiManager";

// Event Group Bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static constexpr int MAXIMUM_RETRY = 5;

// Event Handler
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && 
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            Logger::log(TAG, "Retry Verbindung... (%d/%d)", s_retry_num, MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            Logger::log(TAG, "✗ WLAN-Verbindung fehlgeschlagen nach %d Versuchen", 
                       MAXIMUM_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        Logger::log(TAG, "✓ IP erhalten: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

WiFiManager& WiFiManager::getInstance() {
    static WiFiManager instance;
    return instance;
}

bool WiFiManager::initialize() {
    Logger::log(TAG, "Initialisiere WiFi Manager");
    
    // Event Loop erstellen
    if (esp_event_loop_create_default() != ESP_OK) {
        Logger::log(TAG, "✗ Event Loop konnte nicht erstellt werden");
        return false;
    }
    
    // WiFi Event Group erstellen
    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group) {
        Logger::log(TAG, "✗ Event Group konnte nicht erstellt werden");
        return false;
    }
    
    // Netzwerk-Interface initialisieren
    if (esp_netif_init() != ESP_OK) {
        Logger::log(TAG, "✗ ESP-NetIF konnte nicht initialisiert werden");
        return false;
    }
    
    // Station Interface erstellen
    esp_netif_create_default_wifi_sta();
    
    // WiFi mit Standard-Config initialisieren
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        Logger::log(TAG, "✗ WiFi Init fehlgeschlagen");
        return false;
    }
    
    // Event Handler registrieren
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                       &event_handler, nullptr, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                       &event_handler, nullptr, &instance_got_ip);
    
    _initialized = true;
    Logger::log(TAG, "✓ WiFi Manager initialisiert");
    return true;
}

bool WiFiManager::connect(const char* ssid, const char* password) {
    if (!_initialized) {
        Logger::log(TAG, "✗ WiFi Manager nicht initialisiert");
        return false;
    }
    
    Logger::log(TAG, "Verbinde mit SSID: %s", ssid);
    
    // WiFi Config vorbereiten
    wifi_config_t wifi_config = {};
    std::strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    std::strncpy((char*)wifi_config.sta.password, password, 
                 sizeof(wifi_config.sta.password) - 1);
    
    // Zusätzliche Parameter für robuste Verbindung
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    // WiFi Mode setzen
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        Logger::log(TAG, "✗ WiFi Mode setzen fehlgeschlagen");
        return false;
    }
    
    // Konfiguration anwenden
    if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK) {
        Logger::log(TAG, "✗ WiFi Config setzen fehlgeschlagen");
        return false;
    }
    
    // WiFi starten
    if (esp_wifi_start() != ESP_OK) {
        Logger::log(TAG, "✗ WiFi Start fehlgeschlagen");
        return false;
    }
    
    Logger::log(TAG, "Warte auf Verbindung...");
    
    // Warte auf Verbindung oder Timeout (10 Sekunden)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE, pdFALSE,
                                          pdMS_TO_TICKS(10000));
    
    if (bits & WIFI_CONNECTED_BIT) {
        _connected = true;
        Logger::log(TAG, "✓ WiFi verbunden!");
        return true;
    } else {
        Logger::log(TAG, "✗ WiFi Verbindung timeout");
        return false;
    }
}

bool WiFiManager::isConnected() {
    if (!_initialized) return false;
    
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        _connected = true;
        return true;
    }
    _connected = false;
    return false;
}

std::string WiFiManager::getIpAddress() {
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) return "0.0.0.0";
    
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info);
    
    char ip_str[16];
    esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
    
    return std::string(ip_str);
}

int8_t WiFiManager::getSignalStrength() {
    if (!isConnected()) return -120;
    
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;  // dBm
    }
    return -120;
}

bool WiFiManager::startAccessPoint(const char* ssid, const char* password) {
    Logger::log(TAG, "Starte Access Point: %s", ssid);
    
    // AP Interface erstellen
    esp_netif_create_default_wifi_ap();
    
    // WiFi Config vorbereiten
    wifi_config_t wifi_config = {};
    std::strncpy((char*)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    std::strncpy((char*)wifi_config.ap.password, password, 
                 sizeof(wifi_config.ap.password) - 1);
    
    wifi_config.ap.ssid_len = std::strlen(ssid);
    wifi_config.ap.channel = 6;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.beacon_interval = 100;
    
    // WiFi Mode: AP + STA
    if (esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK) {
        Logger::log(TAG, "✗ AP Mode setzen fehlgeschlagen");
        return false;
    }
    
    // AP-Config anwenden
    if (esp_wifi_set_config(WIFI_IF_AP, &wifi_config) != ESP_OK) {
        Logger::log(TAG, "✗ AP Config setzen fehlgeschlagen");
        return false;
    }
    
    Logger::log(TAG, "✓ Access Point gestartet auf %s", ssid);
    Logger::log(TAG, "   IP: 192.168.4.1");
    
    return true;
}

void WiFiManager::disconnect() {
    if (_initialized) {
        esp_wifi_disconnect();
        esp_wifi_stop();
        _connected = false;
        Logger::log(TAG, "WiFi getrennt");
    }
}
