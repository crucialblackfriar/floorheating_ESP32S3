#include "relay_control.h"
#include "pin_definitions.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "logging/logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "RelayControl";

RelayControl& RelayControl::getInstance() {
    static RelayControl instance;
    return instance;
}

bool RelayControl::initialize() {
    Logger::log(TAG, "Initialisiere Relais-Steuerung");
    
    // Konfiguriere Ventil-Relais
    for (int i = 0; i < NUM_HEIZKREISE; i++) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL << RELAY_PINS[i]);
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        
        gpio_config(&io_conf);
        gpio_set_level((gpio_num_t)RELAY_PINS[i], 0);  // Initial: aus
    }
    
    // Konfiguriere Pumpen-Relais
    gpio_config_t pump_conf = {};
    pump_conf.intr_type = GPIO_INTR_DISABLE;
    pump_conf.mode = GPIO_MODE_OUTPUT;
    pump_conf.pin_bit_mask = (1ULL << PUMP_RELAY_GPIO);
    pump_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    pump_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    gpio_config(&pump_conf);
    gpio_set_level((gpio_num_t)PUMP_RELAY_GPIO, 0);  // Initial: aus
    
    _valve_states.resize(NUM_HEIZKREISE, false);
    _valve_pulse_end_time.resize(NUM_HEIZKREISE, 0);
    _initialized = true;
    
    Logger::log(TAG, "✓ Relais-Steuerung initialisiert (%d Ventile + 1 Pumpe)", 
                NUM_HEIZKREISE);
    return true;
}

void RelayControl::setValve(uint8_t heizkreis_id, bool open) {
    if (!_initialized || heizkreis_id >= NUM_HEIZKREISE) {
        Logger::log(TAG, "✗ Ungültige Heizkreis-ID: %d", heizkreis_id);
        return;
    }
    
    uint8_t gpio_num = RELAY_PINS[heizkreis_id];
    uint8_t signal = open ? 1 : 0;
    
    gpio_set_level((gpio_num_t)gpio_num, signal);
    _valve_states[heizkreis_id] = open;
    
    Logger::log(TAG, "Ventil %d: %s", heizkreis_id, open ? "OFFEN" : "GESCHLOSSEN");
}

void RelayControl::setPump(bool on) {
    if (!_initialized) return;
    
    uint8_t signal = on ? 1 : 0;
    gpio_set_level((gpio_num_t)PUMP_RELAY_GPIO, signal);
    _pump_state = on;
    
    Logger::log(TAG, "Heizkreispumpe: %s", on ? "EIN" : "AUS");
}

bool RelayControl::getValveState(uint8_t heizkreis_id) {
    if (heizkreis_id >= NUM_HEIZKREISE) return false;
    return _valve_states[heizkreis_id];
}

bool RelayControl::getPumpState() {
    return _pump_state;
}

void RelayControl::emergencyShutdown() {
    Logger::log(TAG, "🚨 NOTFALL-ABSCHALTUNG: Schließe alle Ventile!");
    
    // Alle Ventile sofort schließen
    for (int i = 0; i < NUM_HEIZKREISE; i++) {
        setValve(i, false);
    }
    
    // Pumpe mit Verzögerung ausschalten (Auskühlschutz)
    vTaskDelay(pdMS_TO_TICKS(PUMP_SHUTDOWN_DELAY_MS));
    setPump(false);
}

void RelayControl::pulseValve(uint8_t heizkreis_id, uint32_t duration_ms) {
    if (!_initialized || heizkreis_id >= NUM_HEIZKREISE) {
        return;
    }
    
    // Ventil öffnen
    setValve(heizkreis_id, true);
    
    // Speichere End-Zeit für Puls
    _valve_pulse_end_time[heizkreis_id] = 
        esp_timer_get_time() / 1000 + duration_ms;
    
    Logger::log(TAG, "Ventil %d: Puls für %u ms", heizkreis_id, duration_ms);
}
