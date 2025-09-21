/**
 * @file wifi_manager.h
 * @brief ESP-IDF WiFi Manager with tzapu/WiFiManager compatible API
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 * @inspired_by tzapu/WiFiManager (https://github.com/tzapu/WiFiManager)
 *
 * MIT License
 *
 * Copyright (c) 2025 Peter Stangsdal
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ESP-IDF implementation inspired by tzapu/WiFiManager Arduino library
 * Provides Arduino-style API compatibility for ESP-IDF projects
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief WiFi connection status - compatible with tzapu WiFiManager
     */
    typedef enum
    {
        WIFI_STATUS_DISCONNECTED = 0,
        WIFI_STATUS_CONNECTING,
        WIFI_STATUS_CONNECTED,
        WIFI_STATUS_AP_MODE,
        WIFI_STATUS_CONFIG_PORTAL,
        WIFI_STATUS_FAILED
    } wifi_status_t;

    /**
     * @brief WiFi Manager instance structure (opaque)
     */
    typedef struct wifi_manager_t wifi_manager_t;

    /**
     * @brief WiFi event callback function type (like tzapu setAPCallback)
     * @param status Current WiFi status
     * @param ip_address IP address when connected (only valid when status is WIFI_STATUS_CONNECTED)
     */
    typedef void (*wifi_event_callback_t)(wifi_status_t status, const char *ip_address);

    /**
     * @brief Config mode callback (like tzapu setAPCallback)
     * @param wm WiFi Manager instance
     */
    typedef void (*config_mode_callback_t)(wifi_manager_t *wm);

    /**
     * @brief Save config callback (like tzapu setSaveConfigCallback)
     */
    typedef void (*save_config_callback_t)(void);

    /**
     * @brief Initialize WiFi Manager (like tzapu WiFiManager constructor)
     * @return wifi_manager_t* WiFi Manager instance
     */
    wifi_manager_t *wifi_manager_create(void);

    /**
     * @brief Destroy WiFi Manager instance
     * @param wm WiFi Manager instance
     */
    void wifi_manager_destroy(wifi_manager_t *wm);

    /**
     * @brief Auto connect to WiFi - main tzapu-style method
     * Will try to connect to saved WiFi, or start config portal if failed
     * @param wm WiFi Manager instance
     * @param ap_name Access Point name for config portal (default: "ESP32-Setup")
     * @param ap_password Access Point password (NULL for open, min 8 chars)
     * @return true if connected, false if timeout or portal aborted
     */
    bool wifi_manager_auto_connect(wifi_manager_t *wm, const char *ap_name, const char *ap_password);

    /**
     * @brief Start configuration portal on demand (like tzapu startConfigPortal)
     * @param wm WiFi Manager instance
     * @param ap_name Access Point name
     * @param ap_password Access Point password (NULL for open)
     * @return true if connected, false if timeout or aborted
     */
    bool wifi_manager_start_config_portal(wifi_manager_t *wm, const char *ap_name, const char *ap_password);

    /**
     * @brief Set config mode callback (like tzapu setAPCallback)
     * @param wm WiFi Manager instance
     * @param callback Callback function called when entering config mode
     */
    void wifi_manager_set_ap_callback(wifi_manager_t *wm, config_mode_callback_t callback);

    /**
     * @brief Set save config callback (like tzapu setSaveConfigCallback)
     * @param wm WiFi Manager instance
     * @param callback Callback function called when config is saved
     */
    void wifi_manager_set_save_config_callback(wifi_manager_t *wm, save_config_callback_t callback);

    /**
     * @brief Set configuration portal timeout (like tzapu setConfigPortalTimeout)
     * @param wm WiFi Manager instance
     * @param timeout_seconds Timeout in seconds (0 = no timeout)
     */
    void wifi_manager_set_config_portal_timeout(wifi_manager_t *wm, uint32_t timeout_seconds);

    /**
     * @brief Set minimum signal quality for network filtering (like tzapu setMinimumSignalQuality)
     * @param wm WiFi Manager instance
     * @param quality Minimum signal quality percentage (0-100)
     */
    void wifi_manager_set_minimum_signal_quality(wifi_manager_t *wm, int quality);

    /**
     * @brief Enable/disable debug output (like tzapu setDebugOutput)
     * @param wm WiFi Manager instance
     * @param debug true to enable debug output
     */
    void wifi_manager_set_debug_output(wifi_manager_t *wm, bool debug);

    /**
     * @brief Get current WiFi status
     * @param wm WiFi Manager instance
     * @return Current WiFi status
     */
    wifi_status_t wifi_manager_get_status(wifi_manager_t *wm);

    /**
     * @brief Get current IP address (when connected)
     * @param wm WiFi Manager instance
     * @return IP address string or NULL if not connected
     */
    const char *wifi_manager_get_ip_address(wifi_manager_t *wm);

    /**
     * @brief Get config portal SSID
     * @param wm WiFi Manager instance
     * @return Config portal SSID
     */
    const char *wifi_manager_get_config_portal_ssid(wifi_manager_t *wm);

    /**
     * @brief Disconnect from WiFi and erase stored credentials (like tzapu erase)
     * @param wm WiFi Manager instance
     * @return ESP_OK on success
     */
    esp_err_t wifi_manager_erase_config(wifi_manager_t *wm);

    /* ==========================================
     *          CONFIGURATION MANAGEMENT
     * ========================================== */

    /**
     * @brief Add a custom configuration parameter for the web portal
     * @param wm WiFi Manager instance
     * @param key Parameter key (e.g., "mqtt_broker")
     * @param label Human-readable label for web UI
     * @param default_value Default value for the parameter
     * @param required Whether the parameter is required
     * @param placeholder Placeholder text for web UI
     * @return ESP_OK on success
     */
    esp_err_t wifi_manager_add_parameter(wifi_manager_t *wm, const char *key, const char *label,
                                         const char *default_value, bool required, const char *placeholder);

    /**
     * @brief Set a configuration parameter value
     * @param wm WiFi Manager instance
     * @param key Parameter key
     * @param value Parameter value
     * @return ESP_OK on success, ESP_ERR_NOT_FOUND if parameter doesn't exist
     */
    esp_err_t wifi_manager_set_parameter(wifi_manager_t *wm, const char *key, const char *value);

    /**
     * @brief Get a configuration parameter value
     * @param wm WiFi Manager instance
     * @param key Parameter key
     * @param value Buffer to store the value
     * @param value_len Size of value buffer
     * @return ESP_OK on success, ESP_ERR_NOT_FOUND if parameter doesn't exist
     */
    esp_err_t wifi_manager_get_parameter(wifi_manager_t *wm, const char *key, char *value, size_t value_len);

    /**
     * @brief Get configuration parameter as integer
     * @param wm WiFi Manager instance
     * @param key Parameter key
     * @param value Pointer to store integer value
     * @return ESP_OK on success
     */
    esp_err_t wifi_manager_get_parameter_int(wifi_manager_t *wm, const char *key, int *value);

    /**
     * @brief Get configuration parameter as boolean
     * @param wm WiFi Manager instance
     * @param key Parameter key
     * @param value Pointer to store boolean value
     * @return ESP_OK on success
     */
    esp_err_t wifi_manager_get_parameter_bool(wifi_manager_t *wm, const char *key, bool *value);

    /**
     * @brief Save all configuration parameters to persistent storage
     * @param wm WiFi Manager instance
     * @return ESP_OK on success
     */
    esp_err_t wifi_manager_save_config(wifi_manager_t *wm);

    /**
     * @brief Load configuration parameters from persistent storage
     * @param wm WiFi Manager instance
     * @return ESP_OK on success
     */
    esp_err_t wifi_manager_load_config(wifi_manager_t *wm);

    /**
     * @brief Reset all configuration parameters to defaults
     * @param wm WiFi Manager instance
     * @return ESP_OK on success
     */
    esp_err_t wifi_manager_reset_config(wifi_manager_t *wm);

    // Legacy API compatibility (your original functions)
    esp_err_t wifi_manager_init(wifi_event_callback_t callback);
    esp_err_t wifi_manager_start(void);
    wifi_status_t wifi_manager_get_current_status(void);
    const char *wifi_manager_get_current_ip(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */