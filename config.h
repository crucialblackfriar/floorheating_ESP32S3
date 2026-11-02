#pragma once

#include <stdint.h>

// ============= HARDWARE-PINS =============
#define NUM_HEIZKREISE 6
#define NUM_RÜCKLAUF_SENSOREN NUM_HEIZKREISE
#define ONE_WIRE_BUS_GPIO 4
#define PUMP_RELAY_GPIO 12
#define RELAY_BASE_GPIO 13  // Relais 0-5 auf GPIO 13-18

// ============= WIFI & WEBSERVER =============
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"
#define WEBSERVER_PORT 80
#define API_UPDATE_INTERVAL_MS 5000

// ============= FUNKTHERMOMETER (Mobile Alerts MA 10200) =============
#define MA10200_RECEIVER_GPIO 16  // RX Pin für 868 MHz Empfänger
#define MA10200_CHANNEL_OUTDOOR 1  // Kanal für Außenthermometer
#define MA10200_UPDATE_TIMEOUT_MS 300000  // 5 Minuten Timeout

// ============= TEMPERATURREGELUNG =============
#define SETPOINT_MIN_CELSIUS 16.0f
#define SETPOINT_MAX_CELSIUS 28.0f
#define HYSTERESIS_ROOM_TEMP 0.5f  // Hysterese für Raumtemperaturregler

// ============= VENTILSTEUERUNG & DYNAMIK =============
#define VALVE_DEAD_TIME_MS 60000  // 60 Sekunden Totzeit der Ventile
#define VALVE_RESPONSE_TIME_MS 120000  // Gesamte Ventilöffnungszeit ca. 2 Min
#define PUMP_SHUTDOWN_DELAY_MS 30000  // Verzögerung vor Pumpenschaltung

// ============= HEIZGRADKURVE =============
#define HEATING_CURVE_OUTDOOR_MIN -15.0f  // Minimale Außentemperatur
#define HEATING_CURVE_OUTDOOR_MAX 20.0f   // Maximale Außentemperatur
#define HEATING_CURVE_FLOW_MIN 20.0f      // Minimale Vorlauftemperatur
#define HEATING_CURVE_FLOW_MAX 60.0f      // Maximale Vorlauftemperatur
#define HEATING_CURVE_SHIFT 0.5f           // Verschiebung der Kurve

// ============= DYNAMISCHE ZEITKONSTANTEN-SCHÄTZUNG =============
#define TIME_CONSTANT_ESTIMATION_INTERVAL_MS 30000  // Alle 30 Sekunden
#define TIME_CONSTANT_MIN_SECONDS 120  // Minimum 2 Minuten
#define TIME_CONSTANT_MAX_SECONDS 600  // Maximum 10 Minuten
#define MIN_FLOW_CHANGE_FOR_ESTIMATION 0.1f  // Mindeständerung für Schätzung

// ============= LOGGING =============
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2
#define LOG_LEVEL_ERROR 3
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

// ============= SICHERHEIT & GRENZEN =============
#define MAX_ROOM_TEMP_SETPOINT 28.0f
#define MIN_ROOM_TEMP_SETPOINT 16.0f
#define MAX_VALVE_TEMP_DIFFERENTIAL 5.0f  // Maximale Differenz Soll-Ist
#define THERMAL_OVERLOAD_THRESHOLD 70.0f  // Maximale Vorlauftemperatur

// ============= NVS SPEICHER =============
#define NVS_NAMESPACE "fussbodenheiz"
#define NVS_KEY_SETPOINTS "setpoints"
#define NVS_KEY_TIME_CONSTANTS "time_constants"
#define NVS_KEY_VALVE_CALIBRATION "valve_cal"

//#endif // CONFIG_H
